// Author: Antonio Sidenko (Tonetfal), June 2025

#include "GameplayTagManagerModule.h"

#include "GameplayDebugger.h"
#include "Debug/GameplayDebuggerCategory_GameplayTags.h"
#include "Engine/Console.h"

const static FName DebuggerCategoryName = "Gameplay Tags";

void FGameplayTagManagerModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	const auto* CategoryInstance =
		&GameplayTagManager::FGameplayDebuggerCategory_GameplayTags::MakeInstance;
	const auto Category = IGameplayDebugger::FOnGetCategory::CreateStatic(CategoryInstance);

	auto& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory(DebuggerCategoryName, Category,
		EGameplayDebuggerCategoryState::Disabled);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif

	UConsole::RegisterConsoleAutoCompleteEntries.AddLambda([](TArray<FAutoCompleteCommand>& AutoCompleteList)
	{
		const auto* ConsoleSettings = GetDefault<UConsoleSettings>();
		FAutoCompleteCommand& AutoCompleteCommand = AutoCompleteList.AddDefaulted_GetRef();

		AutoCompleteCommand.Command = TEXT("ShowDebug GameplayTagManager");
		AutoCompleteCommand.Desc = TEXT("Toggles debug information for Gameplay Tag Manager");
		AutoCompleteCommand.Color = ConsoleSettings->AutoCompleteCommandColor;
	});
}

void FGameplayTagManagerModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.UnregisterCategory(DebuggerCategoryName);
#endif
}

IMPLEMENT_MODULE(FGameplayTagManagerModule, GameplayTagManager)
