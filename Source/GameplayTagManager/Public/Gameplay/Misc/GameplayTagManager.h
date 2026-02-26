// Author: Antonio Sidenko (Tonetfal), June 2025

#pragma once

#include "GameplayTagContainer.h"
#include "GTM_GameplayTagStackContainer.h"

#include "GameplayTagManager.generated.h"

DECLARE_LOG_CATEGORY_CLASS(LogGameplayTagManager, All, All);

/**
 * General purpose component that can be used to keep track of tags on a component.
 *
 * It's similar to Ability System Component, but it's a standalone component offering similar functionality.
 *
 * It presents Replicated, Loose and Authoritative tags you can add/remove.
 *
 * - Replicated tags: can be only changed server-side,
 *		and will be automatically replicated to everyone.
 *
 * - Loose tags: can be changed by anyone, and will never replicate.
 *
 * - Authoritative tags: can be only changed by server or autonomous proxy,
 *		and will be only replicated to simulated proxies. Useful for prediction.
 *
 * All tags are uniformly advertised as "Tags". You can query or listen for tags changes no matter their nature.
 *
 * All tag types support counting. Negative values are invalid. Count 0 will immediately remove the tag.
 *
 * Regardless the amount of a given tag in any type, "Tags" and delegates will only fire when
 * that tag makes its first appearance or gets removed entirely from every single type.
 */
UCLASS(Category="Gameplay", meta=(BlueprintSpawnableComponent))
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

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
		FOnTagChangedMulticastSignature,
		UGameplayTagManager*, Manager,
		FGameplayTag, Tag,
		bool, bIsPresent);

	DECLARE_DELEGATE_ThreeParams(
		FOnTagChangedSimpleSignature,
		UGameplayTagManager* Manager,
		FGameplayTag Tag,
		bool bIsPresent);

	DECLARE_MULTICAST_DELEGATE_ThreeParams(
		FOnTagChangedMulticastSimpleSignature,
		UGameplayTagManager* Manager,
		FGameplayTag Tag,
		bool bIsPresent);

public:
	UGameplayTagManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	static UGameplayTagManager* Get(const AActor* Actor);

	//~UActorComponent Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void InitializeComponent() override;
	//~End of UActorComponent Interface

	UFUNCTION(BlueprintPure, Category="Gameplay Tags", meta=(BlueprintThreadSafe))
	FGameplayTagContainer GetTags() const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags", meta=(BlueprintThreadSafe))
	const TMap<FGameplayTag, int32>& GetTagsToCount() const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags", meta=(BlueprintThreadSafe))
	int32 GetTagCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	bool HasTag(FGameplayTag Tag, bool bExact = true) const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	bool HasTags(FGameplayTagContainer Tags, bool bExact = true) const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags")
	bool HasAllTags(FGameplayTagContainer Tags, bool bExact = true) const;

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags",
		meta=(AdvancedDisplay="bFireDelegate", Keywords="add assign event"))
	void BindGameplayTagListener(UPARAM(DisplayName="Event") FOnTagChangedSignature Delegate, FGameplayTag Tag,
		bool bFireDelegate = false);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags", meta=(Keywords="remove unassign event"))
	void UnbindGameplayTagListener(UPARAM(DisplayName="Event") FOnTagChangedSignature Delegate, FGameplayTag Tag);

	FDelegateHandle BindGameplayTagListener(FOnTagChangedSimpleSignature Delegate, FGameplayTag Tag);
	void UnbindGameplayTagListener(FDelegateHandle Handle);

#pragma region Replicated
	UFUNCTION(BlueprintPure, Category="Gameplay Tags|Replicated", meta=(BlueprintThreadSafe))
	FGameplayTagContainer GetReplicatedTags() const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags|Replicated", meta=(BlueprintThreadSafe))
	int32 GetReplicatedTagCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void AddTag(FGameplayTag Tag);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void AddTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void RemoveTag(FGameplayTag Tag);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void RemoveTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void OverrideTag(FGameplayTag Tag, int32 NewCount);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void OverrideTags(FGameplayTagContainer Tags, int32 NewCount);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void ClearTag(FGameplayTag Tag);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void ClearTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void ChangeTag(FGameplayTag Tag, bool bAdd);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category="Gameplay Tags|Replicated")
	void ChangeTags(FGameplayTagContainer Tags, bool bAdd);
#pragma endregion

#pragma region Loose
	UFUNCTION(BlueprintPure, Category="Gameplay Tags|Loose", meta=(BlueprintThreadSafe))
	FGameplayTagContainer GetLooseTags() const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags|Loose", meta=(BlueprintThreadSafe))
	int32 GetLooseTagCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void AddLooseTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void AddLooseTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void RemoveLooseTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void RemoveLooseTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void OverrideLooseTag(FGameplayTag Tag, int32 NewCount);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void OverrideLooseTags(FGameplayTagContainer Tags, int32 NewCount);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void ClearLooseTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void ClearLooseTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void ChangeLooseTag(FGameplayTag Tag, bool bAdd);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Loose")
	void ChangeLooseTags(FGameplayTagContainer Tags, bool bAdd);
#pragma endregion

#pragma region Authoritative
	UFUNCTION(BlueprintPure, Category="Gameplay Tags|Authoritative", meta=(BlueprintThreadSafe))
	FGameplayTagContainer GetAuthoritativeTags() const;

	UFUNCTION(BlueprintPure, Category="Gameplay Tags|Authoritative", meta=(BlueprintThreadSafe))
	int32 GetAuthoritativeTagCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void AddAuthoritativeTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void AddAuthoritativeTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void RemoveAuthoritativeTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void RemoveAuthoritativeTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void OverrideAuthoritativeTag(FGameplayTag Tag, int32 NewCount);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void OverrideAuthoritativeTags(FGameplayTagContainer Tags, int32 NewCount);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void ClearAuthoritativeTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void ClearAuthoritativeTags(FGameplayTagContainer Tags);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void ChangeAuthoritativeTag(FGameplayTag Tag, bool bAdd);

	UFUNCTION(BlueprintCallable, Category="Gameplay Tags|Authoritative")
	void ChangeAuthoritativeTags(FGameplayTagContainer Tags, bool bAdd);
#pragma endregion

private:
	void NotifyTagsChanged();
	void CacheTags();

public:
	UPROPERTY(BlueprintAssignable, DisplayName="On Tags Changed", Category="Gameplay Tags")
	FOnTagsChangedSignature OnTagsChangedDelegate;
	FOnTagsChangedSimpleSignature OnTagsChangeSimpleDelegate;

private:
	/** Replicated to everyone. Changed server-side only. */
	UPROPERTY(Replicated)
	FGTM_GameplayTagStackContainer ReplicatedStateTagsContainer;

	/** Locally predicted. Not replicated at all. */
	FGTM_GameplayTagStackContainer LooseStateTagsContainer;

	/** Replicated to everyone but autonomous proxy. Autonomous proxy is changing that on its own. */
	UPROPERTY(Replicated)
	FGTM_GameplayTagStackContainer AuthoritativeStateTagsContainer;

	/**
	 * Global container that counts replicated, loose and authoritative tags all together
	 * making it quicker to query information down the line.
	 */
	TMap<FGameplayTag, int32> CachedTagsCount;
	FGameplayTagContainer CachedTags;

	TMap<FGameplayTag, FOnTagChangedMulticastSignature> SingleListeners;
	TMap<FGameplayTag, FOnTagChangedMulticastSimpleSignature> SingleSimpleListeners;
	FGameplayTagContainer LastKnownTags;
};
