// Author: Antonio Sidenko (Tonetfal), June 2025

#pragma once

#include "dbgLog.h"

#define LOGVS(Args, Msg, ...) LOGVSC(this, Msg, __VA_ARGS__)
#define LOGVSC(CONTEXT, Args, Msg, ...) dbgLOGV(Args.WCO(CONTEXT), Msg, __VA_ARGS__)
#define LOGV(Args, Msg, ...) dbgLOGV(Args, Msg, __VA_ARGS__)

class FGameplayTagManagerModule
	: public IModuleInterface
{
public:
	//~IModuleInterface Interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End of IModuleInterface Interface
};
