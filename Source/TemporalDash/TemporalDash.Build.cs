// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TemporalDash : ModuleRules
{
	public TemporalDash(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TemporalDash",
			"TemporalDash/Variant_Horror",
			"TemporalDash/Variant_Horror/UI",
			"TemporalDash/Variant_Shooter",
			"TemporalDash/Variant_Shooter/AI",
			"TemporalDash/Variant_Shooter/UI",
			"TemporalDash/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
