using UnrealBuildTool;
using System.Collections.Generic;

public class UnfriendBlurTarget : TargetRules
{
	public UnfriendBlurTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("UnfriendBlur");
	}
}
