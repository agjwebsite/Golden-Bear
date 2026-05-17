using UnrealBuildTool;
using System.Collections.Generic;

public class FatManSimulatorTarget : TargetRules
{
	public FatManSimulatorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FatManSimulator");
	}
}
