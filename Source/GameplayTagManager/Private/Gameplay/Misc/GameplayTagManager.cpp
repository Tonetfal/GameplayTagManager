// Author: Antonio Sidenko (Tonetfal), June 2025

#include "Gameplay/Misc/GameplayTagManager.h"

#include "GameplayTagManagerModule.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Profiling/GTM_Profiling.h"

namespace
{
	FGameplayTagContainer RemoveTags(const FGameplayTagContainer& Target, const FGameplayTagContainer& Filter)
	{
		FGameplayTagContainer ResultContainer = Target;
		ResultContainer.RemoveTags(Filter);
		return ResultContainer;
	}
}

UGameplayTagManager::UGameplayTagManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ReplicatedStateTagsContainer(this, "Replicated")
	, LooseStateTagsContainer(this, "Loose")
	, AuthoritativeStateTagsContainer(this, "Authoritative")
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bWantsInitializeComponent = true;

	SetIsReplicatedByDefault(true);
}

UGameplayTagManager* UGameplayTagManager::Get(const AActor* Actor)
{
	if (!ensure(IsValid(Actor)))
	{
		return nullptr;
	}

	auto* TagManager = Actor->GetComponentByClass<UGameplayTagManager>();
	return TagManager;
}

void UGameplayTagManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedStateTagsContainer, Params);

	Params.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, AuthoritativeStateTagsContainer, Params);
}

void UGameplayTagManager::InitializeComponent()
{
	Super::InitializeComponent();

	ReplicatedStateTagsContainer.OnInternalsChangedDelegate.BindUObject(this, &ThisClass::NotifyTagsChanged);
	LooseStateTagsContainer.OnInternalsChangedDelegate.BindUObject(this, &ThisClass::NotifyTagsChanged);
	AuthoritativeStateTagsContainer.OnInternalsChangedDelegate.BindUObject(this, &ThisClass::NotifyTagsChanged);
}

FGameplayTagContainer UGameplayTagManager::GetTags() const
{
	return CachedTags;
}

const TMap<FGameplayTag, int32>& UGameplayTagManager::GetTagsToCount() const
{
	return CachedTagsCount;
}

int32 UGameplayTagManager::GetTagCount(FGameplayTag Tag) const
{
	const int32* FoundCount = CachedTagsCount.Find(Tag);
	return FoundCount ? *FoundCount : 0;
}

bool UGameplayTagManager::HasTag(FGameplayTag Tag, bool bExact) const
{
	return HasTags(FGameplayTagContainer(Tag), bExact);
}

bool UGameplayTagManager::HasTags(FGameplayTagContainer Tags, bool bExact) const
{
	const FGameplayTagContainer AllTags = GetTags();
	const bool bReturnValue = bExact ? AllTags.HasAnyExact(Tags) : AllTags.HasAny(Tags);
	return bReturnValue;
}

bool UGameplayTagManager::HasAllTags(FGameplayTagContainer Tags, bool bExact) const
{
	const FGameplayTagContainer AllTags = GetTags();
	const bool bReturnValue = bExact ? AllTags.HasAllExact(Tags) : AllTags.HasAll(Tags);
	return bReturnValue;
}

void UGameplayTagManager::BindGameplayTagListener(FOnTagChangedSignature Delegate, FGameplayTag Tag, bool bFireDelegate)
{
	auto& MulticastDelegate = SingleListeners.FindOrAdd(Tag);
	if (ensureAlwaysMsgf(!MulticastDelegate.Contains(Delegate),
		TEXT("Binding same delegate to a dynamic delegate is incorrect. Fix your higher level code")))
	{
		MulticastDelegate.Add(Delegate);
	}

	if (bFireDelegate)
	{
		Delegate.ExecuteIfBound(this, Tag, HasTag(Tag));
	}
}

void UGameplayTagManager::UnbindGameplayTagListener(FOnTagChangedSignature Delegate, FGameplayTag Tag)
{
	auto* Signatures = SingleListeners.Find(Tag);
	if (Signatures)
	{
		Signatures->Remove(Delegate);
	}
}

FDelegateHandle UGameplayTagManager::BindGameplayTagListener(FOnTagChangedSimpleSignature Delegate, FGameplayTag Tag)
{
	auto& Signatures = SingleSimpleListeners.FindOrAdd(Tag);
	return Signatures.Add(Delegate);
}

void UGameplayTagManager::UnbindGameplayTagListener(FDelegateHandle Handle)
{
	for (auto& [Tag, MulticastDelegate] : SingleSimpleListeners)
	{
		if (MulticastDelegate.Remove(Handle))
		{
			return;
		}
	}
}

FGameplayTagContainer UGameplayTagManager::GetReplicatedTags() const
{
	return ReplicatedStateTagsContainer.GetTags();
}

int32 UGameplayTagManager::GetReplicatedTagCount(FGameplayTag Tag) const
{
	return ReplicatedStateTagsContainer.GetStackCount(Tag);
}

void UGameplayTagManager::AddTag(FGameplayTag Tag)
{
	if (!ensureMsgf(GetOwner()->HasAuthority(), TEXT("Replicated tags must be changed server-side only")))
	{
		return;
	}

	ReplicatedStateTagsContainer.AddStack(Tag, 1);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedStateTagsContainer, this);
}

void UGameplayTagManager::AddTags(FGameplayTagContainer Tags)
{
	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		AddTag(Tag);
	}
}

void UGameplayTagManager::RemoveTag(FGameplayTag Tag)
{
	if (!ensureMsgf(GetOwner()->HasAuthority(), TEXT("Replicated tags must be changed server-side only")))
	{
		return;
	}

	ReplicatedStateTagsContainer.RemoveStack(Tag, 1);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedStateTagsContainer, this);
}

void UGameplayTagManager::RemoveTags(FGameplayTagContainer Tags)
{
	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		RemoveTag(Tag);
	}
}

void UGameplayTagManager::OverrideTag(FGameplayTag Tag, int32 NewCount)
{
	OverrideTags(FGameplayTagContainer(Tag), NewCount);
}

void UGameplayTagManager::OverrideTags(FGameplayTagContainer Tags, int32 NewCount)
{
	if (!ensureMsgf(GetOwner()->HasAuthority(), TEXT("Replicated tags must be changed server-side only")))
	{
		return;
	}

	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		ReplicatedStateTagsContainer.OverrideStack(Tag, NewCount);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedStateTagsContainer, this);
	}
}

void UGameplayTagManager::ClearTag(FGameplayTag Tag)
{
	OverrideTag(Tag, 0);
}

void UGameplayTagManager::ClearTags(FGameplayTagContainer Tags)
{
	OverrideTags(Tags, 0);
}

void UGameplayTagManager::ChangeTag(FGameplayTag Tag, bool bAdd)
{
	if (bAdd)
	{
		AddTag(Tag);
	}
	else
	{
		RemoveTag(Tag);
	}
}

void UGameplayTagManager::ChangeTags(FGameplayTagContainer Tags, bool bAdd)
{
	if (bAdd)
	{
		AddTags(Tags);
	}
	else
	{
		RemoveTags(Tags);
	}
}

FGameplayTagContainer UGameplayTagManager::GetLooseTags() const
{
	return LooseStateTagsContainer.GetTags();
}

int32 UGameplayTagManager::GetLooseTagCount(FGameplayTag Tag) const
{
	return LooseStateTagsContainer.GetStackCount(Tag);
}

void UGameplayTagManager::AddLooseTag(FGameplayTag Tag)
{
	LooseStateTagsContainer.AddStack(Tag, 1);
}

void UGameplayTagManager::AddLooseTags(FGameplayTagContainer Tags)
{
	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		AddLooseTag(Tag);
	}
}

void UGameplayTagManager::RemoveLooseTag(FGameplayTag Tag)
{
	LooseStateTagsContainer.RemoveStack(Tag, 1);
}

void UGameplayTagManager::RemoveLooseTags(FGameplayTagContainer Tags)
{
	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		RemoveLooseTag(Tag);
	}
}

void UGameplayTagManager::OverrideLooseTag(FGameplayTag Tag, int32 NewCount)
{
	LooseStateTagsContainer.OverrideStack(Tag, NewCount);
}

void UGameplayTagManager::OverrideLooseTags(FGameplayTagContainer Tags, int32 NewCount)
{
	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		OverrideLooseTag(Tag, NewCount);
	}
}

void UGameplayTagManager::ClearLooseTag(FGameplayTag Tag)
{
	OverrideLooseTag(Tag, 0);
}

void UGameplayTagManager::ClearLooseTags(FGameplayTagContainer Tags)
{
	OverrideLooseTags(Tags, 0);
}

void UGameplayTagManager::ChangeLooseTag(FGameplayTag Tag, bool bAdd)
{
	if (bAdd)
	{
		AddLooseTag(Tag);
	}
	else
	{
		RemoveLooseTag(Tag);
	}
}

void UGameplayTagManager::ChangeLooseTags(FGameplayTagContainer Tags, bool bAdd)
{
	if (bAdd)
	{
		AddLooseTags(Tags);
	}
	else
	{
		RemoveLooseTags(Tags);
	}
}

FGameplayTagContainer UGameplayTagManager::GetAuthoritativeTags() const
{
	return AuthoritativeStateTagsContainer.GetTags();
}

int32 UGameplayTagManager::GetAuthoritativeTagCount(FGameplayTag Tag) const
{
	return AuthoritativeStateTagsContainer.GetStackCount(Tag);
}

void UGameplayTagManager::AddAuthoritativeTag(FGameplayTag Tag)
{
	const bool bIsNotSimProxy = GetOwner()->GetLocalRole() != ROLE_SimulatedProxy;
	if (!ensureMsgf(bIsNotSimProxy, TEXT("Authoritative tags can be changed only by server or autonomous proxy")))
	{
		return;
	}

	AuthoritativeStateTagsContainer.AddStack(Tag, 1);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, AuthoritativeStateTagsContainer, this);
}

void UGameplayTagManager::AddAuthoritativeTags(FGameplayTagContainer Tags)
{
	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		AddAuthoritativeTag(Tag);
	}
}

void UGameplayTagManager::RemoveAuthoritativeTag(FGameplayTag Tag)
{
	const bool bIsNotSimProxy = GetOwner()->GetLocalRole() != ROLE_SimulatedProxy;
	if (!ensureMsgf(bIsNotSimProxy, TEXT("Authoritative tags can be changed only by server or autonomous proxy")))
	{
		return;
	}

	AuthoritativeStateTagsContainer.RemoveStack(Tag, 1);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, AuthoritativeStateTagsContainer, this);
}

void UGameplayTagManager::RemoveAuthoritativeTags(FGameplayTagContainer Tags)
{
	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		RemoveAuthoritativeTag(Tag);
	}
}

void UGameplayTagManager::OverrideAuthoritativeTag(FGameplayTag Tag, int32 NewCount)
{
	AuthoritativeStateTagsContainer.OverrideStack(Tag, NewCount);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, AuthoritativeStateTagsContainer, this);
}

void UGameplayTagManager::OverrideAuthoritativeTags(FGameplayTagContainer Tags, int32 NewCount)
{
	const TArray<FGameplayTag>& TagsArray = Tags.GetGameplayTagArray();
	for (const FGameplayTag& Tag : TagsArray)
	{
		OverrideAuthoritativeTag(Tag, NewCount);
	}
}

void UGameplayTagManager::ClearAuthoritativeTag(FGameplayTag Tag)
{
	OverrideLooseTag(Tag, 0);
}

void UGameplayTagManager::ClearAuthoritativeTags(FGameplayTagContainer Tags)
{
	OverrideLooseTags(Tags, 0);
}

void UGameplayTagManager::ChangeAuthoritativeTag(FGameplayTag Tag, bool bAdd)
{
	if (bAdd)
	{
		AddAuthoritativeTag(Tag);
	}
	else
	{
		RemoveAuthoritativeTag(Tag);
	}
}

void UGameplayTagManager::ChangeAuthoritativeTags(FGameplayTagContainer Tags, bool bAdd)
{
	if (bAdd)
	{
		AddAuthoritativeTags(Tags);
	}
	else
	{
		RemoveAuthoritativeTags(Tags);
	}
}

void UGameplayTagManager::NotifyTagsChanged()
{
	SCOPE_CYCLE_COUNTER(STAT_GTM_BroadcastingTags);

	CacheTags();

	const FGameplayTagContainer Tags = GetTags();
	const FGameplayTagContainer AddedTags = ::RemoveTags(Tags, LastKnownTags);
	const FGameplayTagContainer RemovedTags = ::RemoveTags(LastKnownTags, Tags);

	LastKnownTags = Tags;

	OnTagsChangeSimpleDelegate.Broadcast(this, AddedTags, RemovedTags);
	OnTagsChangedDelegate.Broadcast(this, AddedTags, RemovedTags);

	FGameplayTagContainer ModifiedTags;
	ModifiedTags.AppendTags(AddedTags);
	ModifiedTags.AppendTags(RemovedTags);

	const TArray<FGameplayTag>& ModifiedTagsArray = ModifiedTags.GetGameplayTagArray();
	for (const FGameplayTag& It : ModifiedTagsArray)
	{
		const auto SingleListenersCopy = SingleListeners;
		for (auto& [Tag, Listeners] : SingleListenersCopy)
		{
			if (It.MatchesTag(Tag))
			{
				Listeners.Broadcast(this, It, Tags.HasTagExact(It));
			}
		}

		const auto SingleSimpleListenersCopy = SingleSimpleListeners;
		for (auto& [Tag, Listeners] : SingleSimpleListenersCopy)
		{
			if (It.MatchesTag(Tag))
			{
				Listeners.Broadcast(this, It, Tags.HasTagExact(It));
			}
		}
	}
}

void UGameplayTagManager::CacheTags()
{
	CachedTagsCount.Reset();
	CachedTags.Reset();

	CachedTagsCount.Append(ReplicatedStateTagsContainer.GetTagToCountMap());
	CachedTagsCount.Append(LooseStateTagsContainer.GetTagToCountMap());
	CachedTagsCount.Append(AuthoritativeStateTagsContainer.GetTagToCountMap());

	CachedTags.AppendTags(ReplicatedStateTagsContainer.GetTags());
	CachedTags.AppendTags(LooseStateTagsContainer.GetTags());
	CachedTags.AppendTags(AuthoritativeStateTagsContainer.GetTags());
}
