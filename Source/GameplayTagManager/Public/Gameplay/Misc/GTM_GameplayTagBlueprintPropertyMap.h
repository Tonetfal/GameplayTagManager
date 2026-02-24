// Author: Antonio Sidenko (Tonetfal), February 2026

#pragma once

#include "GameplayTagContainer.h"
#include "UObject/Object.h"

#include "GTM_GameplayTagBlueprintPropertyMap.generated.h"

class UGameplayTagManager;

/**
 * Struct used to update a blueprint property with a gameplay tag count.
 * The property is automatically updated as the gameplay tag count changes.
 * It only supports booleans.
 */
USTRUCT()
struct GAMEPLAYTAGMANAGER_API FGTM_GameplayTagBlueprintPropertyMapping
{
	GENERATED_BODY()

public:
	FGTM_GameplayTagBlueprintPropertyMapping() = default;
	FGTM_GameplayTagBlueprintPropertyMapping(const FGTM_GameplayTagBlueprintPropertyMapping& Other);

	// Pretty weird that the assignment operator copies the handle when the copy constructor doesn't - bug?
	FGTM_GameplayTagBlueprintPropertyMapping& operator=(const FGTM_GameplayTagBlueprintPropertyMapping& Other) = default;

public:
	/** Gameplay tag being counted. */
	UPROPERTY(EditAnywhere, Category="Gameplay Tag Blueprint Property")
	FGameplayTag TagToMap;

	/** Property to update with the gameplay tag count. */
	UPROPERTY(VisibleAnywhere, Category="Gameplay Tag Blueprint Property")
	TFieldPath<FProperty> PropertyToEdit;

	/** Name of property being edited. */
	UPROPERTY(VisibleAnywhere, Category="Gameplay Tag Blueprint Property")
	FName PropertyName;

	/** Guid of property being edited. */
	UPROPERTY(VisibleAnywhere, Category="Gameplay Tag Blueprint Property")
	FGuid PropertyGuid;

	/** Handle to delegate bound on the ability system component. */
	FDelegateHandle DelegateHandle;
};

USTRUCT()
struct GAMEPLAYTAGMANAGER_API FGTM_GameplayTagBlueprintPropertyMap
{
	GENERATED_BODY()

public:
	using ThisClass = FGTM_GameplayTagBlueprintPropertyMap;

public:
	FGTM_GameplayTagBlueprintPropertyMap() = default;
	FGTM_GameplayTagBlueprintPropertyMap(const FGTM_GameplayTagBlueprintPropertyMap& Other);
	~FGTM_GameplayTagBlueprintPropertyMap();

	/** Call this to initialize and bind the properties with the gameplay tag manager. */
	void Initialize(UObject* InOwner, UGameplayTagManager* InGameplayTagManager);

	/** Call to manually apply the current tag state, can handle cases where callbacks were skipped */
	void ApplyCurrentTags();

#if WITH_EDITOR
	/** This can optionally be called in the owner's IsDataValid() for data validation. */
	EDataValidationResult IsDataValid(const UObject* ContainingAsset, FDataValidationContext& Context) const;
#endif

protected:
	void Unregister();
	bool IsPropertyTypeValid(const FProperty* Property) const;

	void GameplayTagEventCallback(UGameplayTagManager* Manager, FGameplayTag Tag, bool bIsPresent);

protected:
	UPROPERTY(EditAnywhere, Category="Gameplay Tag Blueprint Property")
	TArray<FGTM_GameplayTagBlueprintPropertyMapping> PropertyMappings;

	TWeakObjectPtr<UObject> CachedOwner;
	TWeakObjectPtr<UGameplayTagManager> CachedGameplayTagManager;
};
