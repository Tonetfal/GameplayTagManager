// Author: Antonio Sidenko (Tonetfal), June 2025

#pragma once

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UObject/Object.h"

#include "GTM_GameplayTagStackContainer.generated.h"

USTRUCT(BlueprintType)
struct FGTM_GameplayTagStack
	: public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	FGTM_GameplayTagStack() = default;
	FGTM_GameplayTagStack(const FGameplayTag& InTag, int32 InStackCount);

	FString ToString() const;

	bool operator==(const FGTM_GameplayTagStack& Rhs) const;

public:
	UPROPERTY(VisibleInstanceOnly)
	FGameplayTag Tag;

	UPROPERTY(VisibleInstanceOnly)
	int32 StackCount = 0;
};

/**
 * Container of gameplay tag stacks.
 */
USTRUCT(BlueprintType)
struct FGTM_GameplayTagStackContainer
	: public FFastArraySerializer
{
	GENERATED_BODY()

public:
	FGTM_GameplayTagStackContainer() = default;
	FGTM_GameplayTagStackContainer(UActorComponent* InOwner, const FString& InType);

	// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	void AddStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	void RemoveStack(FGameplayTag Tag, int32 StackCount);

	// Override a specified tag with the given stack count
	void OverrideStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	int32 GetStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	bool ContainsTag(FGameplayTag Tag) const;
	const TMap<FGameplayTag, int32>& GetTagToCountMap() const;
	FGameplayTagContainer GetTags() const;

	//~FFastArraySerializer Contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer Contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);

private:
	void AddNewStackImpl(FGameplayTag InTag, int32 InStackCount);
	void SetExistingStackCountImpl(FGTM_GameplayTagStack& InStack, int32 InNewCount);
	void RemoveStackImpl(FGTM_GameplayTagStack& InStack);

	void OnStackAdded(const FGTM_GameplayTagStack& InStack);
	void OnStackChanged(const FGTM_GameplayTagStack& InStack, int32 OldCount);
	void OnStackRemoved(const FGTM_GameplayTagStack& InStack);

	void BroadcastStateChanged();

public:
	FSimpleDelegate OnInternalsChangedDelegate;

private:
	// Replicated list of gameplay tag stacks
	UPROPERTY(VisibleInstanceOnly)
	TArray<FGTM_GameplayTagStack> Stacks;

	// Accelerated list of tag stacks for queries
	UPROPERTY(VisibleInstanceOnly, NotReplicated)
	TMap<FGameplayTag, int32> TagToCountMap;

	UPROPERTY(VisibleInstanceOnly, NotReplicated)
	FGameplayTagContainer Tags;

	bool bHasChangedAnything = false;

	TWeakObjectPtr<UActorComponent> Owner = nullptr;
	FString Type;
};

template<>
struct TStructOpsTypeTraits<FGTM_GameplayTagStackContainer>
	: public TStructOpsTypeTraitsBase2<FGTM_GameplayTagStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
