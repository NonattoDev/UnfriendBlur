using UnrealBuildTool;

public class UnfriendBlur : ModuleRules
{
	public UnfriendBlur(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"InputCore",
			"NetCore",
			"SlateCore",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Slate"
		});
	}
}
