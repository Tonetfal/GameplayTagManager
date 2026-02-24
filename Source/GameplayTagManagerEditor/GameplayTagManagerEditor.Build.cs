// Author: Antonio Sidenko (Tonetfal), February 2026

using UnrealBuildTool;

public class GameplayTagManagerEditor : ModuleRules
{
    public GameplayTagManagerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "GameplayTagManager",
                "InputCore",
                "Slate",
                "SlateCore",
            }
        );
    }
}
