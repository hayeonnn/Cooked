// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Cooked : ModuleRules
{
	public Cooked(ReadOnlyTargetRules Target) : base(Target)
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
			"Cooked",
			"Cooked/Variant_Platforming",
			"Cooked/Variant_Platforming/Animation",
			"Cooked/Variant_Combat",
			"Cooked/Variant_Combat/AI",
			"Cooked/Variant_Combat/Animation",
			"Cooked/Variant_Combat/Gameplay",
			"Cooked/Variant_Combat/Interfaces",
			"Cooked/Variant_Combat/UI",
			"Cooked/Variant_SideScrolling",
			"Cooked/Variant_SideScrolling/AI",
			"Cooked/Variant_SideScrolling/Gameplay",
			"Cooked/Variant_SideScrolling/Interfaces",
			"Cooked/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
