using UnrealBuildTool;

public class REAL33DTarget : TargetRules
{
	public REAL33DTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("REAL33D");
	}
}
