// Author: Antonio Sidenko (Tonetfal), June 2025

#pragma once

DECLARE_STATS_GROUP(TEXT("Gameplay Tag Manager"), STATGROUP_GTM, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Adding Tags"), STAT_GTM_AddingTags, STATGROUP_GTM);
DECLARE_CYCLE_STAT(TEXT("Removing Tags"), STAT_GTM_RemovingTags, STATGROUP_GTM);
DECLARE_CYCLE_STAT(TEXT("Overriding Tags"), STAT_GTM_OverridingTags, STATGROUP_GTM);
DECLARE_CYCLE_STAT(TEXT("Broadcasting Tags"), STAT_GTM_BroadcastingTags, STATGROUP_GTM);
