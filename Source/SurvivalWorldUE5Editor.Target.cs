using UnrealBuildTool;
using System.Collections.Generic;

public class SurvivalWorldUE5EditorTarget : TargetRules
{
	public SurvivalWorldUE5EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SurvivalWorldUE5");
	}
}
