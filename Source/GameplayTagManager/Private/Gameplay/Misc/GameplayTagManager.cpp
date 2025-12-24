// Author: Antonio Sidenko (Tonetfal), June 2025

#include "Gameplay/Misc/GameplayTagManager.h"

#include "GameplayTagManagerModule.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UGameplayTagManager::UGameplayTagManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

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

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StateTags, Params);

	Params.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, AuthoritativeStateTags, Params);
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

FGameplayTagContainer UGameplayTagManager::GetTags() const
{
	FGameplayTagContainer ReturnValue;

	ReturnValue.AppendTags(StateTags);
	ReturnValue.AppendTags(LooseStateTags);
	ReturnValue.AppendTags(AuthoritativeStateTags);

	return ReturnValue;
}

FGameplayTagContainer UGameplayTagManager::GetRegularTags() const
{
	return StateTags;
}

FGameplayTagContainer UGameplayTagManager::GetLooseTags() const
{
	return LooseStateTags;
}

FGameplayTagContainer UGameplayTagManager::GetAuthoritativeTags() const
{
	return AuthoritativeStateTags;
}

void UGameplayTagManager::BindGameplayTagListener(FOnTagChangedSignature Delegate, FGameplayTag Tag, bool bFireDelegate)
{
	auto& Signatures = SingleListeners.FindOrAdd(Tag);
	Signatures.Add(Delegate);

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

void UGameplayTagManager::AddTag(FGameplayTag Tag)
{
	AddTags(FGameplayTagContainer(Tag));
}

void UGameplayTagManager::AddTags(FGameplayTagContainer Tags)
{
	check(GetOwner()->HasAuthority());

	Tags.RemoveTags(StateTags);
	if (!StateTags.HasAll(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Add tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Add tags [%s]"), *GetOwner()->GetName(), *Tags.ToString());

		StateTags.AppendTags(Tags);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StateTags, this);
		NotifyTagsChanged();
	}
}

void UGameplayTagManager::RemoveTag(FGameplayTag Tag)
{
	RemoveTags(FGameplayTagContainer(Tag));
}

void UGameplayTagManager::RemoveTags(FGameplayTagContainer Tags)
{
	check(GetOwner()->HasAuthority());

	const FGameplayTagContainer MatchingTags = StateTags.FilterExact(Tags);
	if (StateTags.HasAny(MatchingTags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Remove tags [{0}]", MatchingTags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Remove tags [%s]"), *GetOwner()->GetName(),
			*MatchingTags.ToString());

		StateTags.RemoveTags(MatchingTags);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StateTags, this);
		NotifyTagsChanged();
	}
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

void UGameplayTagManager::AddLooseTag(FGameplayTag Tag)
{
	AddLooseTags(FGameplayTagContainer(Tag));
}

void UGameplayTagManager::AddLooseTags(FGameplayTagContainer Tags)
{
	Tags.RemoveTags(LooseStateTags);
	if (!LooseStateTags.HasAll(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Add loose tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Add loose tags [%s]"), *GetOwner()->GetName(),
			*Tags.ToString());

		LooseStateTags.AppendTags(Tags);
		NotifyTagsChanged();
	}
}

void UGameplayTagManager::RemoveLooseTag(FGameplayTag Tag)
{
	RemoveLooseTags(FGameplayTagContainer(Tag));
}

void UGameplayTagManager::RemoveLooseTags(FGameplayTagContainer Tags)
{
	const FGameplayTagContainer MatchingTags = LooseStateTags.FilterExact(Tags);
	if (LooseStateTags.HasAny(MatchingTags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Remove loose tags [{0}]", MatchingTags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Remove loose tags [%s]"), *GetOwner()->GetName(),
			*MatchingTags.ToString());

		LooseStateTags.RemoveTags(MatchingTags);
		NotifyTagsChanged();
	}
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

void UGameplayTagManager::AddAuthoritativeTag(FGameplayTag Tag)
{
	AddAuthoritativeTags(FGameplayTagContainer(Tag));
}

void UGameplayTagManager::AddAuthoritativeTags(FGameplayTagContainer Tags)
{
	ensure(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy);

	Tags.RemoveTags(AuthoritativeStateTags);
	if (!AuthoritativeStateTags.HasAll(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Add authoritative tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Add authoritative tags [%s]"), *GetOwner()->GetName(),
			*Tags.ToString());

		AuthoritativeStateTags.AppendTags(Tags);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, AuthoritativeStateTags, this);
		NotifyTagsChanged();
	}
}

void UGameplayTagManager::RemoveAuthoritativeTag(FGameplayTag Tag)
{
	RemoveAuthoritativeTags(FGameplayTagContainer(Tag));
}

void UGameplayTagManager::RemoveAuthoritativeTags(FGameplayTagContainer Tags)
{
	ensure(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy);

	const FGameplayTagContainer MatchingTags = AuthoritativeStateTags.FilterExact(Tags);
	if (AuthoritativeStateTags.HasAny(MatchingTags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Remove authoritative tags [{0}]", MatchingTags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Remove authoritative tags [%s]"), *GetOwner()->GetName(),
			*MatchingTags.ToString());

		AuthoritativeStateTags.RemoveTags(MatchingTags);
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, AuthoritativeStateTags, this);
		NotifyTagsChanged();
	}
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

void UGameplayTagManager::OnRep_StateTags()
{
	NotifyTagsChanged();
}

void UGameplayTagManager::OnRep_AuthoritativeStateTags()
{
	NotifyTagsChanged();
}

static FGameplayTagContainer RemoveTags(const FGameplayTagContainer& Target, const FGameplayTagContainer& Filter)
{
	FGameplayTagContainer ResultContainer = Target;
	ResultContainer.RemoveTags(Filter);
	return ResultContainer;
}

void UGameplayTagManager::NotifyTagsChanged()
{
	const FGameplayTagContainer Tags = GetTags();
	const FGameplayTagContainer AddedTags = ::RemoveTags(Tags, LastKnownTags);
	const FGameplayTagContainer RemovedTags = ::RemoveTags(LastKnownTags, Tags);

	LastKnownTags = Tags;

	OnTagsChangeSimpleDelegate.Broadcast(this, AddedTags, RemovedTags);
	OnTagsChangedDelegate.Broadcast(this, AddedTags, RemovedTags);

	FGameplayTagContainer ModifiedTags;
	ModifiedTags.AppendTags(AddedTags);
	ModifiedTags.AppendTags(RemovedTags);

	for (FGameplayTag It : ModifiedTags.GetGameplayTagArray())
	{
		if (auto* FoundListeners = SingleListeners.Find(It))
		{
			TSet<FOnTagChangedSignature> ListenersCopy = *FoundListeners;
			for (FOnTagChangedSignature& Listener : ListenersCopy)
			{
				if (Listener.IsBound())
				{
					FoundListeners->Remove(Listener);
					Listener.Execute(this, It, Tags.HasTagExact(It));
				}
			}
		}
	}
}
