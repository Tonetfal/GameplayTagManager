// Author: Antonio Sidenko (Tonetfal), June 2025

#pragma once

#include "dbgLog.h"

#define LOGVS(Args, Msg, ...) dbgLOGV(Args.WCO(this), Msg, __VA_ARGS__)

class FGameplayTagManagerModule
	: public IModuleInterface
{
public:
	//~IModuleInterface Interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End of IModuleInterface Interface
};
