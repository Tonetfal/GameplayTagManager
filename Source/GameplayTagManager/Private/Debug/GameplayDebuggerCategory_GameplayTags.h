// Author: Antonio Sidenko (Tonetfal), December 2025

#pragma once

#include "GameplayTagContainer.h"

#include "GameplayDebuggerCategory.h"

class UGameplayTagManager;

#if WITH_GAMEPLAY_DEBUGGER
namespace GameplayTagManager
{
	enum class ETagAction : uint8
	{
		Invalid,
		Pushed,
		Popped,
	};

	struct FSerializedTagData
	{
	public:
		void Serialize(FArchive& Ar);

	public:
		FGameplayTag Tag;
		ETagAction Action = ETagAction::Invalid;
		float ActionTimestamp = 0.f;
	};

	struct FSerializedTagManagerData
	{
	public:
		void Serialize(FArchive& Ar);

	public:
		TWeakObjectPtr<const UGameplayTagManager> TagManager = nullptr;

		float FirstSerializationTimestamp = 0.f;
		FString TagManagerOwnerName;
		TArray<FSerializedTagData> SerializedTags;
		FGameplayTagContainer LastTags;
	};

	struct FRepData
	{
	public:
		void Serialize(FArchive& Ar);

	public:
		TArray<FSerializedTagManagerData> DebugData;
	};

	class FGameplayDebuggerCategory_GameplayTags
		: public FGameplayDebuggerCategory
	{
	public:
		FGameplayDebuggerCategory_GameplayTags();

		/** Creates an instance of this category - will be used on module startup to include our category in the Editor */
		static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

		//~FGameplayDebuggerCategory Interface
		virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
		virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;
		virtual void OnDataPackReplicated(int32 DataPackId) override;
		//~End of FGameplayDebuggerCategory Interface

	private:
		FSerializedTagManagerData& GetDebugData(const UGameplayTagManager* TagManager);

	private:
		TWeakObjectPtr<AActor> LastDebugActor = nullptr;
		FRepData DataPack;
	};
}
#endif
