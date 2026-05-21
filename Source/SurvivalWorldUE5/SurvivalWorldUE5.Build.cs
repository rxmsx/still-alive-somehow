using UnrealBuildTool;

public class SurvivalWorldUE5 : ModuleRules
{
	public SurvivalWorldUE5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"InputCore",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"GameplayTags"
		});
	}
}
