// Copyright 2026 Silvan Teufel. All Rights Reserved.

using UnrealBuildTool;

public class PoiseBreak : ModuleRules
{
	public PoiseBreak(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",

			// AActor, UActorComponent and DrawDebugHelpers for the demo level.
			"Engine",

			// UPoiseBreakSettings is a UDeveloperSettings, so the tiers, the regeneration and the
			// hyper-armor scale sit under Project Settings > Plugins > PoiseBreak without an
			// editor module.
			"DeveloperSettings",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

		// Deliberately NOT here:
		//   GameplayAbilities - poise is a number your damage pipeline already knows how to produce.
		//                       Binding the plugin to GAS would shut out every project that does
		//                       combat another way, and the call site is one line either way.
		//   AIModule          - PoiseBreak decides THAT an actor broke and how hard. What the
		//                       animation and the behaviour tree do about it is your project's.
	}
}
