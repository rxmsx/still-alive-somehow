using UnrealBuildTool;
using System.Collections.Generic;

public class SurvivalWorldUE5Target : TargetRules
{
	public SurvivalWorldUE5Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SurvivalWorldUE5");
	}
}
