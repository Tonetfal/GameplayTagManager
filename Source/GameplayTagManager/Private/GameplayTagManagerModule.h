// Author: Antonio Sidenko (Tonetfal), June 2025

#pragma once

#include "dbgLog.h"

#define LOGVS(Args, Msg, ...) dbgLOGV(Args.WCO(this), Msg, __VA_ARGS__)