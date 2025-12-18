// Author: Antonio Sidenko (Tonetfal), December 2025

#include "GameplayDebuggerCategory_GameplayTags.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "Engine/Canvas.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Misc/GameplayTagManager.h"

namespace GameplayTagManager
{
	FGameplayDebuggerCategory_GameplayTags::FGameplayDebuggerCategory_GameplayTags()
	{
		bShowOnlyWithDebugActor = false;
	}

	TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_GameplayTags::MakeInstance()
	{
		return MakeShareable(new FGameplayDebuggerCategory_GameplayTags());
	}

	static void SerializeTagManager(FSerializedTagManagerData& InOutSerializedData)
	{
		if (!ensure(InOutSerializedData.TagManager.IsValid()))
		{
			return;
		}

		const FGameplayTagContainer NewTags = InOutSerializedData.TagManager->GetTags();

		const float Time = InOutSerializedData.TagManager->GetWorld()->TimeSeconds;
		if (InOutSerializedData.FirstSerializationTimestamp != Time)
		{
			const TArray<FGameplayTag>& NewTagsArray = NewTags.GetGameplayTagArray();
			const TArray<FGameplayTag>& OldTagsArray = InOutSerializedData.LastTags.GetGameplayTagArray();

			for (const FGameplayTag& Tag : NewTagsArray)
			{
				if (!OldTagsArray.Contains(Tag))
				{
					FSerializedTagData Data(Tag, ETagAction::Pushed, Time);
					InOutSerializedData.SerializedTags.EmplaceAt(0, Data);
				}
			}

			for (const FGameplayTag& Tag : OldTagsArray)
			{
				if (!NewTagsArray.Contains(Tag))
				{
					FSerializedTagData Data(Tag, ETagAction::Popped, Time);
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
			Container.RemoveAll([Time](const FSerializedTagData& Data)
			{
				return (Time - Data.ActionTimestamp) > 10.f;
			});
		}

		InOutSerializedData.LastTags = NewTags;
	}

	constexpr FColor White(255, 255, 255);
	constexpr FColor Red(255, 0, 0);
	constexpr FColor Green(0, 255, 0);

	static FColor ActionToColor(ETagAction Action)
	{
		switch (Action)
		{
			case ETagAction::Invalid:
				return White;
			case ETagAction::Pushed:
				return Green;
			case ETagAction::Popped:
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
			DebugData.Reset();
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
			case ETagAction::Pushed:
				return "Pushed";
			case ETagAction::Popped:
				return "Popped";
			default:
				return "Invalid";
		}
	}

	static float PrintGameplayTagManager(const FSerializedTagManagerData& Data,
		FGameplayDebuggerCanvasContext& CanvasContext, float CursorX)
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

		PRINT(FColor::Orange, TEXT("Tag manager owner: %s"), *Data.TagManager->GetOwner()->GetName());

		CanvasContext.MoveToNewLine();
		PRINT(FColor::White, TEXT("Tags:"));
		for (const FGameplayTag& Tag : Data.LastTags.GetGameplayTagArray())
		{
			PRINT(FColor::White, TEXT("- %s"), *Tag.ToString());
		}

		CanvasContext.MoveToNewLine();
		PRINT(FColor::White, TEXT("Tag actions:"));
		for (const FSerializedTagData& SerializedTag : Data.SerializedTags)
		{
			const FString TagString = SerializedTag.Tag.ToString();
			const FString ActionString = LexToString(SerializedTag.Action);

			const float Time = Data.TagManager->GetWorld()->TimeSeconds;
			PRINT(ActionToColor(SerializedTag.Action), TEXT("- %s - %s (%.2fs)"),
				*TagString, *ActionString, Time - SerializedTag.ActionTimestamp);
		}

		return MaxCursorX;
	}

	void FGameplayDebuggerCategory_GameplayTags::DrawData(APlayerController* OwnerPC,
		FGameplayDebuggerCanvasContext& CanvasContext)
	{
		float MaxCursorX = 0.f;
		for (const FSerializedTagManagerData& Data : DebugData)
		{
			MaxCursorX = PrintGameplayTagManager(Data, CanvasContext, MaxCursorX);
			CanvasContext.CursorY = CanvasContext.DefaultY + CanvasContext.GetLineHeight();
			MaxCursorX += 100.f;
		}
	}

	FSerializedTagManagerData& FGameplayDebuggerCategory_GameplayTags::GetDebugData(
		const UGameplayTagManager* TagManager)
	{
		auto* FoundData = DebugData.FindByPredicate([TagManager](const FSerializedTagManagerData& Data)
		{
			return Data.TagManager == TagManager;
		});

		if (FoundData)
		{
			return *FoundData;
		}

		const float Time = TagManager->GetWorld()->TimeSeconds;
		return DebugData.Emplace_GetRef(FSerializedTagManagerData(TagManager, Time));
	}
}
#endif
