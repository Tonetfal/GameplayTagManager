// Author: Antonio Sidenko (Tonetfal), February 2026

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGameplayTagManagerEditorModule
	: public IModuleInterface
{
public:
    //~IModuleInterface Interface
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    //~End of IModuleInterface Interface
};
