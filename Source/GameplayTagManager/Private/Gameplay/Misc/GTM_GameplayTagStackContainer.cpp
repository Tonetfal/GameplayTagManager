// Author: Antonio Sidenko (Tonetfal), June 2025

#include "Gameplay/Misc/GTM_GameplayTagStackContainer.h"

#include "GameplayTagManagerModule.h"
#include "Gameplay/Misc/GameplayTagManager.h"
#include "Profiling/GTM_Profiling.h"

FGTM_GameplayTagStack::FGTM_GameplayTagStack(const FGameplayTag& InTag, int32 InStackCount)
	: Tag(InTag)
	, StackCount(InStackCount)
{
}

FString FGTM_GameplayTagStack::ToString() const
{
	return FString::Printf(TEXT("Tag: %s | Count: %d"), *Tag.ToString(), StackCount);
}

bool FGTM_GameplayTagStack::operator==(const FGTM_GameplayTagStack& Rhs) const
{
	return Tag == Rhs.Tag;
}

FGTM_GameplayTagStackContainer::FGTM_GameplayTagStackContainer(UActorComponent* InOwner, const FString& InType)
	: Owner(InOwner)
	, Type(InType)
{
}

void FGTM_GameplayTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
	if (!ensureMsgf(Tag.IsValid(), TEXT("An invalid tag was passed to AddStack")))
	{
		return;
	}

	if (!ensureMsgf(StackCount > 0, TEXT("An invalid count was passed to AddStack")))
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_GTM_AddingTags);

	if (Tags.HasTagExact(Tag))
	{
		// Add to existing stack
		for (FGTM_GameplayTagStack& Stack : Stacks)
		{
			if (Stack.Tag == Tag)
			{
				const int32 NewCount = Stack.StackCount + StackCount;
				SetExistingStackCountImpl(Stack, NewCount);
				return;
			}
		}
	}
	else
	{
		// Create a new one
		AddNewStackImpl(Tag, StackCount);
	}
}

void FGTM_GameplayTagStackContainer::RemoveStack(FGameplayTag Tag, int32 StackCount)
{
	if (!ensureMsgf(Tag.IsValid(), TEXT("An invalid tag was passed to RemoveStack")))
	{
		return;
	}

	if (!ensureMsgf(StackCount > 0, TEXT("An invalid count was passed to RemoveStack")))
	{
		return;
	}

	if (!Tags.HasTagExact(Tag))
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_GTM_RemovingTags);

	for (auto It = Stacks.CreateIterator(); It; ++It)
	{
		FGTM_GameplayTagStack& Stack = *It;
		if (Stack.Tag == Tag)
		{
			if (Stack.StackCount == StackCount)
			{
				FGTM_GameplayTagStack StackCopy = Stack;
				It.RemoveCurrent();
				RemoveStackImpl(StackCopy);
			}
			else
			{
				const int32 NewCount = Stack.StackCount - StackCount;
				SetExistingStackCountImpl(Stack, NewCount);
			}

			return;
		}
	}
}

void FGTM_GameplayTagStackContainer::OverrideStack(FGameplayTag Tag, int32 StackCount)
{
	if (!ensureMsgf(Tag.IsValid(), TEXT("An invalid tag was passed to OverrideStack")))
	{
		return;
	}

	if (!ensureMsgf(StackCount >= 0, TEXT("An invalid count was passed to OverrideStack")))
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_GTM_OverridingTags);

	const int32* FoundCount = TagToCountMap.Find(Tag);
	if (FoundCount)
	{
		if (*FoundCount == StackCount)
		{
			// Avoid overriding with the same count
			return;
		}

		// Override existing one
		for (auto It = Stacks.CreateIterator(); It; ++It)
		{
			FGTM_GameplayTagStack& Stack = *It;
			if (Stack.Tag == Tag)
			{
				if (StackCount == 0)
				{
					FGTM_GameplayTagStack StackCopy = Stack;
					It.RemoveCurrent();
					RemoveStackImpl(StackCopy);
				}
				else
				{
					SetExistingStackCountImpl(Stack, StackCount);
				}

				return;
			}
		}
	}
	else
	{
		if (StackCount == 0)
		{
			// We have no tag, and the stack count isn't positive
			return;
		}

		// Create a new one
		AddNewStackImpl(Tag, StackCount);
	}
}

int32 FGTM_GameplayTagStackContainer::GetStackCount(FGameplayTag Tag) const
{
	return TagToCountMap.FindRef(Tag);
}

bool FGTM_GameplayTagStackContainer::ContainsTag(FGameplayTag Tag) const
{
	return TagToCountMap.Contains(Tag);
}

const TMap<FGameplayTag, int32>& FGTM_GameplayTagStackContainer::GetTagToCountMap() const
{
	return TagToCountMap;
}

FGameplayTagContainer FGTM_GameplayTagStackContainer::GetTags() const
{
	return Tags;
}

void FGTM_GameplayTagStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		OnStackRemoved(Stacks[Index]);

		const FGameplayTag Tag = Stacks[Index].Tag;
		TagToCountMap.Remove(Tag);
		Tags.RemoveTag(Tag);
	}

	bHasChangedAnything |= !RemovedIndices.IsEmpty();
}

void FGTM_GameplayTagStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FGTM_GameplayTagStack& Stack = Stacks[Index];
		TagToCountMap.Add(Stack.Tag, Stack.StackCount);
		Tags.AddTag(Stack.Tag);

		OnStackAdded(Stack);
	}

	bHasChangedAnything |= !AddedIndices.IsEmpty();
}

void FGTM_GameplayTagStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FGTM_GameplayTagStack& Stack = Stacks[Index];
		const int32 OldCount = TagToCountMap[Stack.Tag];
		TagToCountMap[Stack.Tag] = Stack.StackCount;

		OnStackChanged(Stack, OldCount);
	}

	bHasChangedAnything |= !ChangedIndices.IsEmpty();
}

bool FGTM_GameplayTagStackContainer::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	const bool bReturnValue = FastArrayDeltaSerialize<FGTM_GameplayTagStack, FGTM_GameplayTagStackContainer>(
		Stacks, DeltaParms, *this);

	if (bHasChangedAnything)
	{
		bHasChangedAnything = false;
		BroadcastStateChanged();
	}

	return bReturnValue;
}

void FGTM_GameplayTagStackContainer::AddNewStackImpl(FGameplayTag InTag, int32 InStackCount)
{
	FGTM_GameplayTagStack& NewStack = Stacks.Emplace_GetRef(InTag, InStackCount);
	MarkItemDirty(NewStack);

	TagToCountMap.Add(InTag, InStackCount);
	Tags.AddTag(InTag);

	OnStackAdded(NewStack);
	BroadcastStateChanged();
}

void FGTM_GameplayTagStackContainer::SetExistingStackCountImpl(FGTM_GameplayTagStack& InStack, int32 InNewCount)
{
	const int32 OldCount = InStack.StackCount;
	InStack.StackCount = InNewCount;
	MarkItemDirty(InStack);

	TagToCountMap[InStack.Tag] = InNewCount;

	OnStackChanged(InStack, OldCount);
	BroadcastStateChanged();
}

void FGTM_GameplayTagStackContainer::RemoveStackImpl(FGTM_GameplayTagStack& InStack)
{
	MarkArrayDirty();

	TagToCountMap.Remove(InStack.Tag);
	Tags.RemoveTag(InStack.Tag);

	OnStackRemoved(InStack);
	BroadcastStateChanged();
}

void FGTM_GameplayTagStackContainer::OnStackAdded(const FGTM_GameplayTagStack& InStack)
{
#if USE_LOGGING_IN_SHIPPING
	LOGVSC(Owner.Get(), .Category(LogGameplayTagManager).VisualLogText(Owner.Get(), false),
		"Add {0} tag [{1}]", Type, InStack);
#endif

	ensure(InStack.StackCount > 0);
}

void FGTM_GameplayTagStackContainer::OnStackChanged(const FGTM_GameplayTagStack& InStack, int32 OldCount)
{
#if USE_LOGGING_IN_SHIPPING
	LOGVSC(Owner.Get(), .Category(LogGameplayTagManager).VisualLogText(Owner.Get(), false),
		"Change {0} tag [{1}]. Old count {2}", Type, InStack, OldCount);
#endif

	ensure(InStack.StackCount > 0);
}

void FGTM_GameplayTagStackContainer::OnStackRemoved(const FGTM_GameplayTagStack& InStack)
{
#if USE_LOGGING_IN_SHIPPING
	LOGVSC(Owner.Get(), .Category(LogGameplayTagManager).VisualLogText(Owner.Get(), false),
		"Remove {0} tag [{1}]", Type, InStack);
#endif

	ensure(InStack.StackCount > 0);
}

void FGTM_GameplayTagStackContainer::BroadcastStateChanged()
{
	OnInternalsChangedDelegate.Execute();
}
