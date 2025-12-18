// Author: Antonio Sidenko (Tonetfal), June 2025

#include "GameplayTagManagerModule.h"

#include "GameplayDebugger.h"
#include "Debug/GameplayDebuggerCategory_GameplayTags.h"

const static FName DebuggerCategoryName = "Gameplay Tags";

void FGameplayTagManagerModule::StartupModule()
{
	const auto* CategoryInstance =
		&GameplayTagManager::FGameplayDebuggerCategory_GameplayTags::MakeInstance;
	const auto Category = IGameplayDebugger::FOnGetCategory::CreateStatic(CategoryInstance);

	auto& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory(DebuggerCategoryName, Category,
		EGameplayDebuggerCategoryState::Disabled);
	GameplayDebuggerModule.NotifyCategoriesChanged();
}

void FGameplayTagManagerModule::ShutdownModule()
{
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.UnregisterCategory(DebuggerCategoryName);
}

IMPLEMENT_MODULE(FGameplayTagManagerModule, GameplayTagManager)
