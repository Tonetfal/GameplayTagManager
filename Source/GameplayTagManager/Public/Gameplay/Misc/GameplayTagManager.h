// Author: Antonio Sidenko (Tonetfal), June 2025

#pragma once

#include "GameplayTagContainer.h"

#include "GameplayTagManager.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogGameplayTagManager, All, All);

UCLASS(meta=(BlueprintSpawnableComponent))
class GAMEPLAYTAGMANAGER_API UGameplayTagManager
	: public UActorComponent
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
		FOnTagsChangedSignature,
		UGameplayTagManager*, Manager,
		FGameplayTagContainer, AddedTags,
		FGameplayTagContainer, RemovedTags);

	DECLARE_DYNAMIC_DELEGATE_ThreeParams(
		FOnTagChangedSignature,
		UGameplayTagManager*, Manager,
		FGameplayTag, Tag,
		bool, bIsPresent);

public:
	UGameplayTagManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	bool HasTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	bool HasTags(FGameplayTagContainer Tags) const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	FGameplayTagContainer GetTags() const;

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void BindGameplayTagListener(UPARAM(DisplayName="Event") FOnTagChangedSignature Delegate, FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void UnbindGameplayTagListener(UPARAM(DisplayName="Event") FOnTagChangedSignature Delegate, FGameplayTag Tag);

#pragma region Replicated
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags")
	void AddTag(FGameplayTag Tag);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags")
	void AddTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags")
	void RemoveTag(FGameplayTag Tag);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags")
	void RemoveTags(FGameplayTagContainer Tags);
#pragma endregion

#pragma region Loose
	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void AddLooseTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void AddLooseTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void RemoveLooseTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void RemoveLooseTags(FGameplayTagContainer Tags);
#pragma endregion

#pragma region Authoritative
	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void AddAuthoritativeTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void AddAuthoritativeTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void RemoveAuthoritativeTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void RemoveAuthoritativeTags(FGameplayTagContainer Tags);
#pragma endregion

private:
	UFUNCTION()
	void OnRep_StateTags();

	UFUNCTION()
	void OnRep_AuthoritativeStateTags();

	void NotifyTagsChanged();

public:
	UPROPERTY(BlueprintAssignable, DisplayName="On Tags Changed", Category="Gameplay Tags")
	FOnTagsChangedSignature OnTagsChangedDelegate;

private:
	/** Replicated to everyone. Changed server-side only. */
	UPROPERTY(ReplicatedUsing="OnRep_StateTags")
	FGameplayTagContainer StateTags;

	/** Locally predicted. Not replicated at all. */
	FGameplayTagContainer LooseStateTags;

	/** Replicated to everyone but owner. Owner (locally controlling client) is changing that on its own. */
	UPROPERTY(ReplicatedUsing="OnRep_StateTags")
	FGameplayTagContainer AuthoritativeStateTags;

	TMap<FGameplayTag, TSet<FOnTagChangedSignature>> SingleListeners;
	FGameplayTagContainer LastKnownTags;
};
