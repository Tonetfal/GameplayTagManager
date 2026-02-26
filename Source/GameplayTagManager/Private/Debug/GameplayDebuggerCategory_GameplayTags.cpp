// Author: Antonio Sidenko (Tonetfal), December 2025

#include "GameplayDebuggerCategory_GameplayTags.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Misc/GameplayTagManager.h"

namespace GameplayTagManager
{
	void FRepSerializedTagData::Serialize(FArchive& Ar)
	{
		Ar << Tag;
		Ar << Action;
		Ar << ActionTimestamp;
		Ar << AdditionalData;
	}

	void FRepSerializedTagManagerData::FGameplayTagCountPair::Serialize(FArchive& Ar)
	{
		Ar << Tag;
		Ar << Count;
	}

	void FRepSerializedTagManagerData::Serialize(FArchive& Ar)
	{
		Ar << FirstSerializationTimestamp;
		Ar << TagManagerOwnerName;

		if (Ar.IsSaving())
		{
			int32 Num = SerializedTags.Num();
			Ar << Num;

			for (FRepSerializedTagData& Data : SerializedTags)
			{
				Data.Serialize(Ar);
			}
		}
		else
		{
			int32 Num;
			Ar << Num;

			SerializedTags.Reset(Num);
			for (int32 i = 0; i < Num; ++i)
			{
				SerializedTags.AddDefaulted_GetRef().Serialize(Ar);
			}
		}

		if (Ar.IsSaving())
		{
			int32 Num = LastTags.Num();
			Ar << Num;

			for (auto& Pair : LastTags)
			{
				FGameplayTag Tag = Pair.Key;
				int32 Value = Pair.Value;

				Ar << Tag;
				Ar << Value;
			}
		}
		else
		{
			int32 Num;
			Ar << Num;

			LastTags.Reset();
			LastTags.Reserve(Num);
			for (int32 i = 0; i < Num; ++i)
			{
				FGameplayTag Tag;
				int32 Value;

				Ar << Tag;
				Ar << Value;

				LastTags.Emplace(Tag, Value);
			}
		}
	}

	void FRepData::Serialize(FArchive& Ar)
	{
		int32 Num = DebugData.Num();
		Ar << Num;

		if (Ar.IsLoading())
		{
			DebugData.SetNum(Num);
		}

		for (int32 i = 0; i < Num; i++)
		{
			DebugData[i].Serialize(Ar);
		}
	}

	FGameplayDebuggerCategory_GameplayTags::FGameplayDebuggerCategory_GameplayTags()
	{
		bShowOnlyWithDebugActor = false;

		SetDataPackReplication<FRepData>(&DataPack, EGameplayDebuggerDataPack::ResetOnActorChange);
	}

	TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_GameplayTags::MakeInstance()
	{
		return MakeShareable(new FGameplayDebuggerCategory_GameplayTags());
	}

	static void SerializeTagManager(FRepSerializedTagManagerData& InOutSerializedData)
	{
		if (!ensure(InOutSerializedData.TagManager.IsValid()))
		{
			return;
		}

		const TMap<FGameplayTag, int32>& NewTags = InOutSerializedData.TagManager->GetTagsToCount();

		const float Time = InOutSerializedData.TagManager->GetWorld()->TimeSeconds;
		if (InOutSerializedData.FirstSerializationTimestamp != Time)
		{
			const TMap<FGameplayTag, int32>& NewTagsArray = NewTags;
			const TMap<FGameplayTag, int32>& OldTagsArray = InOutSerializedData.LastTags;

			for (const auto& [Tag, Count] : NewTagsArray)
			{
				if (!OldTagsArray.Contains(Tag))
				{
					FRepSerializedTagData Data(Tag, ETagAction::Added, Time, "0 -> " + FString::FromInt(Count));
					InOutSerializedData.SerializedTags.EmplaceAt(0, Data);
				}
			}

			for (const auto& [Tag, NewCount] : NewTagsArray)
			{
				const auto* OldCount = OldTagsArray.Find(Tag);
				if (OldCount && NewCount != *OldCount)
				{
					const FString AdditionalData = FString::FromInt(*OldCount) + " -> " + FString::FromInt(NewCount);
					if (NewCount > *OldCount)
					{
						FRepSerializedTagData Data(Tag, ETagAction::Increased, Time, AdditionalData);
						InOutSerializedData.SerializedTags.EmplaceAt(0, Data);
					}
					else
					{
						FRepSerializedTagData Data(Tag, ETagAction::Decreased, Time, AdditionalData);
						InOutSerializedData.SerializedTags.EmplaceAt(0, Data);
					}
				}
			}

			for (const auto& [Tag, Count] : OldTagsArray)
			{
				if (!NewTagsArray.Contains(Tag))
				{
					FRepSerializedTagData Data(Tag, ETagAction::Removed, Time, FString::FromInt(Count) + " -> 0");
					InOutSerializedData.SerializedTags.EmplaceAt(0, Data);
				}
			}

			// Trim the history if it's too long
			constexpr int32 MaxHistoryLength = 10;
			auto& Container = InOutSerializedData.SerializedTags;
			const int32 ToRemove = Container.Num() - MaxHistoryLength;
			if (ToRemove > 0)
			{
				Container.SetNum(MaxHistoryLength);
			}

			// Delete entries that are too old
			Container.RemoveAll([Time](const FRepSerializedTagData& Data)
			{
				return (Time - Data.ActionTimestamp) > 10.f;
			});
		}

		InOutSerializedData.LastTags = NewTags;
	}

	constexpr FColor White(255, 255, 255);
	constexpr FColor Green(0, 255, 0);
	constexpr FColor DarkGreen(0, 192, 0);
	constexpr FColor Orange(255, 128, 0);
	constexpr FColor Red(255, 0, 0);

	static FColor ActionToColor(ETagAction Action)
	{
		switch (Action)
		{
			case ETagAction::Invalid:
				return White;
			case ETagAction::Added:
				return DarkGreen;
			case ETagAction::Increased:
				return Green;
			case ETagAction::Decreased:
				return Orange;
			case ETagAction::Removed:
				return Red;
			default:
				return White;
		}
	}

	void FGameplayDebuggerCategory_GameplayTags::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
	{
		if (LastDebugActor != DebugActor)
		{
			LastDebugActor = DebugActor;
			DataPack.DebugData.Reset();
		}

		if (IsValid(DebugActor))
		{
			const auto* ActorTagManager = DebugActor->FindComponentByClass<UGameplayTagManager>();
			if (IsValid(ActorTagManager))
			{
				SerializeTagManager(GetDebugData(ActorTagManager));
			}

			if (const auto* Pawn = Cast<APawn>(DebugActor))
			{
				if (const auto* Controller = Pawn->GetController())
				{
					const auto* ControllerTagManager = Controller->FindComponentByClass<UGameplayTagManager>();
					if (IsValid(ControllerTagManager))
					{
						SerializeTagManager(GetDebugData(ControllerTagManager));
					}
				}

				if (const auto* PlayerState = Pawn->GetPlayerState())
				{
					const auto* PlayerStateTagManager = PlayerState->FindComponentByClass<UGameplayTagManager>();
					if (IsValid(PlayerStateTagManager))
					{
						SerializeTagManager(GetDebugData(PlayerStateTagManager));
					}
				}
			}
		}
	}

	static FString LexToString(ETagAction Action)
	{
		switch (Action)
		{
			case ETagAction::Added:
				return "Added";
			case ETagAction::Increased:
				return "Increased";
			case ETagAction::Decreased:
				return "Decreased";
			case ETagAction::Removed:
				return "Removed";
			default:
				return "Invalid";
		}
	}

	static float PrintGameplayTagManager(const FRepSerializedTagManagerData& Data,
		FGameplayDebuggerCanvasContext& CanvasContext, float CursorX, float CurrentTime)
	{
		float MaxCursorX = CursorX;

#define PRINT(COLOR, ...) \
		do { \
			const FString String = FString::Printf(__VA_ARGS__); \
			CanvasContext.CursorX = CursorX; \
			CanvasContext.Print(COLOR, String); \
			float SizeX; \
			float SizeY; \
			CanvasContext.MeasureString(String, SizeX, SizeY); \
			MaxCursorX = FMath::Max(MaxCursorX, CursorX + SizeX); \
		} while(false)

		PRINT(FColor::Orange, TEXT("Tag manager owner: %s"), *Data.TagManagerOwnerName);

		CanvasContext.MoveToNewLine();
		PRINT(FColor::White, TEXT("Tags:"));
		for (const auto& [Tag, Count] : Data.LastTags)
		{
			PRINT(FColor::White, TEXT("- %s (%d)"), *Tag.ToString(), Count);
		}

		CanvasContext.MoveToNewLine();
		PRINT(FColor::White, TEXT("Tag actions:"));
		for (const FRepSerializedTagData& SerializedTag : Data.SerializedTags)
		{
			const FString TagString = SerializedTag.Tag.ToString();
			const FString ActionString = LexToString(SerializedTag.Action);

			PRINT(ActionToColor(SerializedTag.Action), TEXT("- %s - %s (%.2fs) %s"),
				*TagString, *ActionString, CurrentTime - SerializedTag.ActionTimestamp, *SerializedTag.AdditionalData);
		}

		return MaxCursorX;
	}

	void FGameplayDebuggerCategory_GameplayTags::DrawData(APlayerController* OwnerPC,
		FGameplayDebuggerCanvasContext& CanvasContext)
	{
		float MaxCursorX = 0.f;
		const float Time = OwnerPC->GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		for (FRepSerializedTagManagerData& Data : DataPack.DebugData)
		{
			MaxCursorX = PrintGameplayTagManager(Data, CanvasContext, MaxCursorX, Time);
			CanvasContext.CursorY = CanvasContext.DefaultY + CanvasContext.GetLineHeight();
			MaxCursorX += 100.f;
		}
	}

	void FGameplayDebuggerCategory_GameplayTags::OnDataPackReplicated(int32 DataPackId)
	{
		MarkRenderStateDirty();
	}

	FRepSerializedTagManagerData& FGameplayDebuggerCategory_GameplayTags::GetDebugData(
		const UGameplayTagManager* TagManager)
	{
		auto* FoundData = DataPack.DebugData.FindByPredicate([TagManager](const FRepSerializedTagManagerData& Data)
		{
			return Data.TagManager == TagManager;
		});

		if (FoundData)
		{
			return *FoundData;
		}

		FRepSerializedTagManagerData& Data = DataPack.DebugData.AddDefaulted_GetRef();
		Data.TagManager = TagManager;
		Data.FirstSerializationTimestamp = TagManager->GetWorld()->TimeSeconds;
		Data.TagManagerOwnerName = TagManager->GetOwner()->GetName();

		return Data;
	}
}
#endif
