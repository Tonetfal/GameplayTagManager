// Author: Antonio Sidenko (Tonetfal), February 2026

#include "Gameplay/Misc/GTM_GameplayTagBlueprintPropertyMap.h"

#include "Gameplay/Misc/GameplayTagManager.h"
#include "GameplayTagManagerModule.h"
#include "Misc/DataValidation.h"

FGTM_GameplayTagBlueprintPropertyMapping::FGTM_GameplayTagBlueprintPropertyMapping(
	const FGTM_GameplayTagBlueprintPropertyMapping& Other)
	: TagToMap(Other.TagToMap)
	, PropertyToEdit(Other.PropertyToEdit)
	, PropertyName(Other.PropertyName)
	, PropertyGuid(Other.PropertyGuid)
	, DelegateHandle() // Don't copy handle
{
}

FGTM_GameplayTagBlueprintPropertyMap::FGTM_GameplayTagBlueprintPropertyMap(
	const FGTM_GameplayTagBlueprintPropertyMap& Other)
{
	ensureMsgf(Other.CachedOwner.IsExplicitlyNull(),
		TEXT("FGTM_GameplayTagBlueprintPropertyMap cannot be used inside an array or other container "
			"that is copied after register!"));
	PropertyMappings = Other.PropertyMappings;
}

FGTM_GameplayTagBlueprintPropertyMap::~FGTM_GameplayTagBlueprintPropertyMap()
{
	Unregister();
}

void FGTM_GameplayTagBlueprintPropertyMap::Initialize(UObject* InOwner, UGameplayTagManager* InGameplayTagManager)
{
	UClass* OwnerClass = IsValid(InOwner) ? InOwner->GetClass() : nullptr;
	if (!IsValid(OwnerClass))
	{
		LOGV(.Category(LogGameplayTagManager).Error(), "FGTM_GameplayTagBlueprintPropertyMap: "
			"Initialize() called with an invalid Owner.");
		return;
	}

	if (!IsValid(InGameplayTagManager))
	{
		LOGV(.Category(LogGameplayTagManager).Error(), "FGTM_GameplayTagBlueprintPropertyMap: "
			"Initialize() called with an invalid GameplayTagManager.");
		return;
	}

	if (CachedOwner == InOwner && CachedGameplayTagManager == InGameplayTagManager)
	{
		// Already initialized
		return;
	}

	if (CachedOwner.IsValid())
	{
		Unregister();
	}

	CachedOwner = InOwner;
	CachedGameplayTagManager = InGameplayTagManager;

	UGameplayTagManager::FOnTagChangedSimpleSignature Delegate;
	Delegate.BindRaw(this, &ThisClass::GameplayTagEventCallback);

	// Process array starting at the end so we can remove invalid entries
	for (int32 MappingIndex = (PropertyMappings.Num() - 1); MappingIndex >= 0; --MappingIndex)
	{
		FGTM_GameplayTagBlueprintPropertyMapping& Mapping = PropertyMappings[MappingIndex];

		if (Mapping.TagToMap.IsValid())
		{
			FProperty* Property = OwnerClass->FindPropertyByName(Mapping.PropertyName);
			if (Property && IsPropertyTypeValid(Property))
			{
				Mapping.PropertyToEdit = Property;
				Mapping.DelegateHandle = CachedGameplayTagManager->BindGameplayTagListener(
					Delegate, Mapping.TagToMap);
				continue;
			}
		}

		// Entry was invalid: Remove it from the array
		LOGV(.Category(LogGameplayTagManager).Error(), "FGTM_GameplayTagBlueprintPropertyMap: "
			"Removing invalid GameplayTagBlueprintPropertyMapping [Index: {0}, Tag:{1}, Property:{2}] for [{3}].",
			MappingIndex, Mapping.TagToMap, Mapping.PropertyName, InOwner->GetName());

		PropertyMappings.RemoveAtSwap(MappingIndex, EAllowShrinking::No);
	}

	// Broadcast all bindings to make sure that our state is correct in case some of the tags were already present
	// before initializating the property map
	for (const FGTM_GameplayTagBlueprintPropertyMapping& PropertyMapping : PropertyMappings)
	{
		Delegate.Execute(CachedGameplayTagManager.Get(), PropertyMapping.TagToMap,
			CachedGameplayTagManager->HasTag(PropertyMapping.TagToMap, true));
	}
}

void FGTM_GameplayTagBlueprintPropertyMap::ApplyCurrentTags()
{
	UObject* Owner = CachedOwner.Get();
	if (!IsValid(Owner))
	{
		LOGV(.Category(LogGameplayTagManager).Warn(), "FGTM_GameplayTagBlueprintPropertyMap::ApplyCurrentTags "
			"called with an invalid Owner.");
		return;
	}

	auto* GameplayTagManager = CachedGameplayTagManager.Get();
	if (!IsValid(GameplayTagManager))
	{
		LOGV(.Category(LogGameplayTagManager).Warn(), "FGTM_GameplayTagBlueprintPropertyMap::ApplyCurrentTags "
			"called with an invalid GameplayTagManager.");
		return;
	}

	for (FGTM_GameplayTagBlueprintPropertyMapping& Mapping : PropertyMappings)
	{
		if (Mapping.PropertyToEdit.Get() && Mapping.TagToMap.IsValid())
		{
			if (const FBoolProperty* BoolProperty = CastField<const FBoolProperty>(Mapping.PropertyToEdit.Get()))
			{
				const bool bHasTag = GameplayTagManager->HasTag(Mapping.TagToMap);
				BoolProperty->SetPropertyValue_InContainer(Owner, bHasTag);
			}
		}
	}
}

#if WITH_EDITOR
EDataValidationResult FGTM_GameplayTagBlueprintPropertyMap::IsDataValid(const UObject* Owner,
	FDataValidationContext& Context) const
{
	UClass* OwnerClass = ((Owner != nullptr) ? Owner->GetClass() : nullptr);
	if (!OwnerClass)
	{
		LOGV(.Category(LogGameplayTagManager).Error(), "FGTM_GameplayTagBlueprintPropertyMap: IsDataValid() "
			"called with an invalid Owner.");
		return EDataValidationResult::Invalid;
	}

	for (const FGTM_GameplayTagBlueprintPropertyMapping& Mapping : PropertyMappings)
	{
		if (!Mapping.TagToMap.IsValid())
		{
			Context.AddError(FText::Format(INVTEXT("The gameplay tag [{0}] for property [{1}] is empty or invalid."),
				FText::AsCultureInvariant(Mapping.TagToMap.ToString()),
				FText::FromName(Mapping.PropertyName)));
		}

		if (FProperty* Property = OwnerClass->FindPropertyByName(Mapping.PropertyName))
		{
			if (!IsPropertyTypeValid(Property))
			{
				Context.AddError(FText::Format(INVTEXT(
						"The property [{0}] for gameplay tag [{1}] is not a supported type.  "
						"Supported types are: boolean."),
					FText::FromName(Mapping.PropertyName),
					FText::AsCultureInvariant(Mapping.TagToMap.ToString())));
			}
		}
		else
		{
			Context.AddError(FText::Format(INVTEXT("The property [{0}] for gameplay tag [{1}] could not be found."),
				FText::FromName(Mapping.PropertyName),
				FText::AsCultureInvariant(Mapping.TagToMap.ToString())));
		}
	}

	return Context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

void FGTM_GameplayTagBlueprintPropertyMap::Unregister()
{
	if (CachedGameplayTagManager.IsValid())
	{
		for (FGTM_GameplayTagBlueprintPropertyMapping& PropertyMapping : PropertyMappings)
		{
			CachedGameplayTagManager->UnbindGameplayTagListener(PropertyMapping.DelegateHandle);
			PropertyMapping.PropertyToEdit = nullptr;
		}
	}

	CachedOwner = nullptr;
	CachedGameplayTagManager = nullptr;
}

bool FGTM_GameplayTagBlueprintPropertyMap::IsPropertyTypeValid(const FProperty* Property) const
{
	check(Property);
	return Property->IsA<FBoolProperty>();
}

void FGTM_GameplayTagBlueprintPropertyMap::GameplayTagEventCallback(UGameplayTagManager* Manager, FGameplayTag Tag,
	bool bIsPresent)
{
	FGTM_GameplayTagBlueprintPropertyMapping* Mapping = PropertyMappings.FindByPredicate(
		[Tag](const FGTM_GameplayTagBlueprintPropertyMapping& Test)
		{
			return (Tag == Test.TagToMap);
		});

	if (Mapping && Mapping->PropertyToEdit.Get())
	{
		if (const FBoolProperty* BoolProperty = CastField<const FBoolProperty>(Mapping->PropertyToEdit.Get()))
		{
			BoolProperty->SetPropertyValue_InContainer(CachedOwner.Get(), bIsPresent);
		}
	}
}
