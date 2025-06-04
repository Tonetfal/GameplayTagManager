// Author: Antonio Sidenko (Tonetfal), June 2025

using UnrealBuildTool;

public class GameplayTagManager : ModuleRules
{
	public GameplayTagManager(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"Slate",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"dbgLog"
			}
		);
	}
}
