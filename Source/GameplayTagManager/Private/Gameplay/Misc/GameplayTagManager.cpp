// Author: Antonio Sidenko (Tonetfal), June 2025

#include "Gameplay/Misc/GameplayTagManager.h"

#include "GameplayTagManagerModule.h"
#include "Net/UnrealNetwork.h"

UGameplayTagManager::UGameplayTagManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

void UGameplayTagManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StateTags);
	DOREPLIFETIME_CONDITION(ThisClass, AuthoritativeStateTags, COND_SkipOwner);
}

bool UGameplayTagManager::HasTag(FGameplayTag Tag) const
{
	return HasTags(FGameplayTagContainer(Tag));
}

bool UGameplayTagManager::HasTags(FGameplayTagContainer Tags) const
{
	const FGameplayTagContainer AllTags = GetTags();
	const bool bReturnValue = AllTags.HasAny(Tags);
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

void UGameplayTagManager::BindGameplayTagListener(FOnTagChangedSignature Delegate, FGameplayTag Tag)
{
	auto& Signatures = SingleListeners.FindOrAdd(Tag);
	Signatures.Add(Delegate);
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

	if (!StateTags.HasAll(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Add tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Add tags [%s]"), *GetOwner()->GetName(), *Tags.ToString());

		StateTags.AppendTags(Tags);
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

	if (StateTags.HasAny(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Remove tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Remove tags [%s]"), *GetOwner()->GetName(), *Tags.ToString());

		StateTags.RemoveTags(Tags);
		NotifyTagsChanged();
	}
}

void UGameplayTagManager::AddLooseTag(FGameplayTag Tag)
{
	AddLooseTags(FGameplayTagContainer(Tag));
}

void UGameplayTagManager::AddLooseTags(FGameplayTagContainer Tags)
{
	if (!LooseStateTags.HasAll(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Add loose tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Add loose tags [%s]"), *GetOwner()->GetName(), *Tags.ToString());

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
	if (LooseStateTags.HasAny(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Remove loose tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Remove loose tags [%s]"), *GetOwner()->GetName(), *Tags.ToString());

		LooseStateTags.RemoveTags(Tags);
		NotifyTagsChanged();
	}
}

void UGameplayTagManager::AddAuthoritativeTag(FGameplayTag Tag)
{
	AddAuthoritativeTags(FGameplayTagContainer(Tag));
}

void UGameplayTagManager::AddAuthoritativeTags(FGameplayTagContainer Tags)
{
	ensure(GetOwner()->GetLocalRole() != ROLE_SimulatedProxy);

	if (!AuthoritativeStateTags.HasAll(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Add authoritative tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Add authoritative tags [%s]"), *GetOwner()->GetName(), *Tags.ToString());

		AuthoritativeStateTags.AppendTags(Tags);
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

	if (AuthoritativeStateTags.HasAny(Tags))
	{
		LOGVS(.Category(LogGameplayTagManager), "Remove authoritative tags [{0}]", Tags);
		UE_VLOG(this, LogGameplayTagManager, Log, TEXT("%s> Remove authoritative tags [%s]"), *GetOwner()->GetName(), *Tags.ToString());

		AuthoritativeStateTags.RemoveTags(Tags);
		NotifyTagsChanged();
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

	OnTagsChangedDelegate.Broadcast(this, AddedTags, RemovedTags);

	FGameplayTagContainer ModifiedTags;
	ModifiedTags.AppendTags(AddedTags);
	ModifiedTags.AppendTags(RemovedTags);

	for (FGameplayTag It : ModifiedTags.GetGameplayTagArray())
	{
		if (auto* FoundListeners = SingleListeners.Find(It))
		{
			TSet<FOnTagChangedSignature> InvalidListenersToRemove;
			for (FOnTagChangedSignature& Listener : *FoundListeners)
			{
				if (!Listener.ExecuteIfBound(this, It, Tags.HasTagExact(It)))
				{
					InvalidListenersToRemove.Add(Listener);
				}
			}

			*FoundListeners = FoundListeners->Difference(InvalidListenersToRemove);
		}
	}

	LastKnownTags = Tags;
}
