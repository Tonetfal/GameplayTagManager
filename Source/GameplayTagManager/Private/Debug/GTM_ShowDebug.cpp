// Author: Antonio Sidenko (Tonetfal), February 2026

#include "Debug/GTM_ShowDebug.h"

#include "DisplayDebugHelpers.h"
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Misc/GameplayTagManager.h"

#if ENABLE_DRAW_DEBUG
namespace
{
	using namespace GameplayTagManager;

	constexpr FColor MainColor(255, 255, 0);
	constexpr FColor White(255, 255, 255);
	constexpr FColor Green(0, 255, 0);
	constexpr FColor DarkGreen(0, 192, 0);
	constexpr FColor Orange(255, 128, 0);
	constexpr FColor Red(255, 0, 0);

	FColor ActionToColor(ETagAction Action)
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

	FString LexToString(ETagAction Action)
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

	void SerializeTagManager(FSerializedTagManagerData_Normal& InOutSerializedData)
	{
		if (!ensure(InOutSerializedData.TagManager.IsValid()))
		{
			return;
		}

		const TMap<FGameplayTag, int32>& NewTags = InOutSerializedData.TagManager->GetTagsToCount();

		const float Time = InOutSerializedData.TagManager->GetWorld()->TimeSeconds;
		if (InOutSerializedData.TrackingStartTimestamp != Time)
		{
			const TMap<FGameplayTag, int32>& NewTagsArray = NewTags;
			const TMap<FGameplayTag, int32>& OldTagsArray = InOutSerializedData.LastTags;

			for (const auto& [Tag, Count] : NewTagsArray)
			{
				if (!OldTagsArray.Contains(Tag))
				{
					FSerializedTagData_Normal& SerializedTag = InOutSerializedData.GetSerializedTag(Tag);
					SerializedTag.LastAction = ETagAction::Added;
					SerializedTag.AdditionalData = "0 -> " + FString::FromInt(Count);
					SerializedTag.LastActionTimestamp = Time;
				}
			}

			for (const auto& [Tag, NewCount] : NewTagsArray)
			{
				const auto* OldCount = OldTagsArray.Find(Tag);
				if (OldCount && NewCount != *OldCount)
				{
					FSerializedTagData_Normal& SerializedTag = InOutSerializedData.GetSerializedTag(Tag);
					SerializedTag.AdditionalData = FString::FromInt(*OldCount) + " -> " + FString::FromInt(NewCount);;
					SerializedTag.LastActionTimestamp = Time;

					if (NewCount > *OldCount)
					{
						SerializedTag.LastAction = ETagAction::Increased;
					}
					else
					{
						SerializedTag.LastAction = ETagAction::Decreased;
					}
				}
			}

			for (const auto& [Tag, Count] : OldTagsArray)
			{
				if (!NewTagsArray.Contains(Tag))
				{
					FSerializedTagData_Normal& SerializedTag = InOutSerializedData.GetSerializedTag(Tag);
					SerializedTag.LastAction = ETagAction::Removed;
					SerializedTag.AdditionalData = FString::FromInt(Count) + " -> 0";
					SerializedTag.LastActionTimestamp = Time;
				}
			}
		}
		else
		{
			for (const auto& [Tag, NewCount] : NewTags)
			{
				FSerializedTagData_Normal& SerializedTag = InOutSerializedData.GetSerializedTag(Tag);
				SerializedTag.LastAction = ETagAction::Invalid;
				SerializedTag.LastActionTimestamp = Time;
			}
		}

		InOutSerializedData.LastTags = NewTags;
	}

	void PrintGameplayTagManager(const FSerializedTagManagerData_Normal& Data, float CurrentTime,
		const FShowDebugContext& InContext, FShowDebugIntermediate& InIntermediateData)
	{
		FDisplayDebugManager& DisplayDebugManager = InContext.Canvas->DisplayDebugManager;

#define PRINT(COLOR, ...) \
		do { \
			DisplayDebugManager.SetDrawColor(COLOR); \
			const FString String = FString::Printf(__VA_ARGS__); \
			DisplayDebugManager.DrawString(String); \
		} while(false)

		PRINT(MainColor, TEXT("  Owner: %s"), *Data.TagManager->GetOwner()->GetFName().ToString());

		for (const FSerializedTagData_Normal& SerializedTag : Data.SerializedTags)
		{
			const FString TagString = SerializedTag.Tag.ToString();
			const FString ActionString = LexToString(SerializedTag.LastAction);

			if (SerializedTag.LastAction == ETagAction::Invalid)
			{
				PRINT(ActionToColor(SerializedTag.LastAction), TEXT("    - %s"), *TagString);
			}
			else
			{
				PRINT(ActionToColor(SerializedTag.LastAction), TEXT("    - %s - %s (%.2fs) %s"),
					*TagString, *ActionString, CurrentTime - SerializedTag.LastActionTimestamp,
					*SerializedTag.AdditionalData);
			}
		}

#undef PRINT
	}
}

namespace GameplayTagManager
{
	FShowDebugContext::FShowDebugContext(AHUD* InHUD, UCanvas* InCanvas, const FDebugDisplayInfo& InDisplayInfo)
		: HUD(InHUD)
		, Canvas(InCanvas)
		, DisplayInfo(InDisplayInfo)
	{
	}

	FShowDebugIntermediate::FShowDebugIntermediate(float& InYL, float& InYPos)
		: YL(InYL)
		, YPos(InYPos)
	{
	}

	bool FSerializedTagData_Normal::operator==(const FSerializedTagData_Normal& Rhs) const
	{
		return Tag == Rhs.Tag;
	}

	FSerializedTagData_Normal& FSerializedTagManagerData_Normal::GetSerializedTag(FGameplayTag InTag)
	{
		int32 FoundIndex = SerializedTags.Find(FSerializedTagData_Normal(InTag));
		if (FoundIndex == INDEX_NONE)
		{
			FoundIndex = SerializedTags.Emplace(FSerializedTagData_Normal(InTag));
		}

		return SerializedTags[FoundIndex];
	}

	void FGTM_ShowDebug::ShowDebugInfo(const FShowDebugContext& InContext, FShowDebugIntermediate& InIntermediateData)
	{
		// ShowDebug is called from each GTM, which causes it to render multiple times;
		// keep track of the frame it was rendered from.
		// Use PIE ID to allow using the debug on every simulation separately
		const int32 EditorID = UE::GetPlayInEditorID();
		uint64& LastHandledFrame = LastHandledFrameAcrossSimulations.FindOrAdd(EditorID);
		if (LastHandledFrame == GFrameCounter)
		{
			return;
		}

		LastHandledFrame = GFrameCounter;
		static const FName NAME_GameplayTagManager = TEXT("GameplayTagManager");
		if (!InContext.DisplayInfo.IsDisplayOn(NAME_GameplayTagManager))
		{
			return;
		}

		AActor* DebugTarget = InContext.HUD->GetCurrentDebugTargetActor();
		CollectData(DebugTarget);
		DrawData(InContext, InIntermediateData);
		LastDebugActor = DebugTarget;
	}

	void FGTM_ShowDebug::CollectData(AActor* InDebugActor)
	{
		if (LastDebugActor != InDebugActor)
		{
			LastDebugActor = InDebugActor;
			DebugData.Reset();
		}

		// @todo It'd be nice if we could inspect not just pawns, but also other actors having GTM

		if (IsValid(InDebugActor))
		{
			const auto* ActorTagManager = InDebugActor->FindComponentByClass<UGameplayTagManager>();
			if (IsValid(ActorTagManager))
			{
				SerializeTagManager(GetDebugData(ActorTagManager));
			}

			if (const auto* Pawn = Cast<APawn>(InDebugActor))
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

	void FGTM_ShowDebug::DrawData(const FShowDebugContext& InContext, FShowDebugIntermediate& InIntermediateData)
	{
		if (!LastDebugActor.IsValid())
		{
			return;
		}

		FDisplayDebugManager& DisplayDebugManager = InContext.Canvas->DisplayDebugManager;
		DisplayDebugManager.SetFont(GEngine->GetSmallFont());
		DisplayDebugManager.SetDrawColor(MainColor);
		DisplayDebugManager.DrawString(TEXT("GAMEPLAY TAG MANAGER"));

		const float Time = LastDebugActor->GetWorld()->GetTimeSeconds();
		for (const FSerializedTagManagerData_Normal& Data : DebugData)
		{
			PrintGameplayTagManager(Data, Time, InContext, InIntermediateData);
		}
	}

	FSerializedTagManagerData_Normal& FGTM_ShowDebug::GetDebugData(const UGameplayTagManager* TagManager)
	{
		auto* FoundData = DebugData.FindByPredicate([TagManager](const FSerializedTagManagerData_Normal& Data)
		{
			return Data.TagManager == TagManager;
		});

		if (FoundData)
		{
			return *FoundData;
		}

		FSerializedTagManagerData_Normal& Data = DebugData.AddDefaulted_GetRef();
		Data.TagManager = TagManager;
		Data.TrackingStartTimestamp = TagManager->GetWorld()->TimeSeconds;

		return Data;
	}
}
#endif
