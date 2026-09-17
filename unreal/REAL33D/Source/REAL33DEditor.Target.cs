using UnrealBuildTool;

public class REAL33DEditorTarget : TargetRules
{
	public REAL33DEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("REAL33D");
	}
}
