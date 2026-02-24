// Author: Antonio Sidenko (Tonetfal), February 2026

#include "GameplayTagManagerEditor.h"

#include "GTM_GameplayTagBlueprintPropertyMappingDetails.h"

#define LOCTEXT_NAMESPACE "FGameplayTagManagerEditorModule"

void FGameplayTagManagerEditorModule::StartupModule()
{
	auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout("GTM_GameplayTagBlueprintPropertyMapping",
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(
			&FGTM_GameplayTagBlueprintPropertyMappingDetails::MakeInstance));
}

void FGameplayTagManagerEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("GTM_GameplayTagBlueprintPropertyMapping");
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameplayTagManagerEditorModule, GameplayTagManagerEditor)
