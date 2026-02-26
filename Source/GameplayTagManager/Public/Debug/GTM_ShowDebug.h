// Author: Antonio Sidenko (Tonetfal), February 2026

#pragma once

#include "GameplayTagContainer.h"

class UGameplayTagManager;

namespace GameplayTagManager
{
	enum class ETagAction : uint8
	{
		Invalid,
		Added,
		Increased,
		Decreased,
		Removed,
	};

#if ENABLE_DRAW_DEBUG
	struct FShowDebugContext
	{
	public:
		FShowDebugContext() = delete;
		FShowDebugContext(AHUD* InHUD, UCanvas* InCanvas, const FDebugDisplayInfo& InDisplayInfo);

	public:
		AHUD* HUD = nullptr;
		UCanvas* Canvas = nullptr;
		const FDebugDisplayInfo& DisplayInfo;
	};

	struct FShowDebugIntermediate
	{
	public:
		FShowDebugIntermediate() = delete;
		FShowDebugIntermediate(float& InYL, float& InYPos);

	public:
		float& YL;
		float& YPos;
	};

	struct FSerializedTagData_Normal
	{
	public:
		bool operator==(const FSerializedTagData_Normal& Rhs) const;

	public:
		FGameplayTag Tag;
		int32 Count = 0;

		ETagAction LastAction = ETagAction::Invalid;
		float LastActionTimestamp = 0.f;

		FString AdditionalData;
	};

	struct FSerializedTagManagerData_Normal
	{
	public:
		FSerializedTagData_Normal& GetSerializedTag(FGameplayTag InTag);

	public:
		TWeakObjectPtr<const UGameplayTagManager> TagManager = nullptr;
		TArray<FSerializedTagData_Normal> SerializedTags;
		TMap<FGameplayTag, int32> LastTags;
		float TrackingStartTimestamp = 0.f;
	};

	struct FGTM_ShowDebug
	{
	public:
		void ShowDebugInfo(const FShowDebugContext& InContext, FShowDebugIntermediate& InIntermediateData);

	private:
		void CollectData(AActor* InDebugActor);
		void DrawData(const FShowDebugContext& InContext, FShowDebugIntermediate& InIntermediateData);

	private:
		FSerializedTagManagerData_Normal& GetDebugData(const UGameplayTagManager* TagManager);

	private:
		inline static TMap<int32, uint64> LastHandledFrameAcrossSimulations;

	private:
		TArray<FSerializedTagManagerData_Normal> DebugData;
		TWeakObjectPtr<AActor> LastDebugActor = nullptr;
	};
#endif
}
