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

	DECLARE_MULTICAST_DELEGATE_ThreeParams(
		FOnTagsChangedSimpleSignature,
		UGameplayTagManager* Manager,
		FGameplayTagContainer AddedTags,
		FGameplayTagContainer RemovedTags);

	DECLARE_DYNAMIC_DELEGATE_ThreeParams(
		FOnTagChangedSignature,
		UGameplayTagManager*, Manager,
		FGameplayTag, Tag,
		bool, bIsPresent);

public:
	UGameplayTagManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	static UGameplayTagManager* Get(const AActor* Actor);

	//~UActorComponent Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of UActorComponent Interface

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	bool HasTag(FGameplayTag Tag, bool bExact = true) const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	bool HasTags(FGameplayTagContainer Tags, bool bExact = true) const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	bool HasAllTags(FGameplayTagContainer Tags, bool bExact = true) const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags", meta=(BlueprintThreadSafe))
	FGameplayTagContainer GetTags() const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags", meta=(BlueprintThreadSafe))
	FGameplayTagContainer GetRegularTags() const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags", meta=(BlueprintThreadSafe))
	FGameplayTagContainer GetLooseTags() const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags", meta=(BlueprintThreadSafe))
	FGameplayTagContainer GetAuthoritativeTags() const;

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags",
		meta=(AdvancedDisplay="bFireDelegate", Keywords="add assign event"))
	void BindGameplayTagListener(UPARAM(DisplayName="Event") FOnTagChangedSignature Delegate, FGameplayTag Tag,
		bool bFireDelegate = false);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags", meta=(Keywords="remove unassign event"))
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

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags")
	void ChangeTag(FGameplayTag Tag, bool bAdd);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags")
	void ChangeTags(FGameplayTagContainer Tags, bool bAdd);
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

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void ChangeLooseTag(FGameplayTag Tag, bool bAdd);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void ChangeLooseTags(FGameplayTagContainer Tags, bool bAdd);
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

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void ChangeAuthoritativeTag(FGameplayTag Tag, bool bAdd);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags")
	void ChangeAuthoritativeTags(FGameplayTagContainer Tags, bool bAdd);
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
	FOnTagsChangedSimpleSignature OnTagsChangeSimpleDelegate;

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
	TSet<FOnTagChangedSignature> InvalidSingleListeners;
	FGameplayTagContainer LastKnownTags;
};
