using UnrealBuildTool;
using System.Collections.Generic;

public class FatManSimulatorEditorTarget : TargetRules
{
	public FatManSimulatorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FatManSimulator");
	}
}
