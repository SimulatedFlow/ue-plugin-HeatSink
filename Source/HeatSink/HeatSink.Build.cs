// Copyright 2026 Silvan Teufel. All Rights Reserved.

using UnrealBuildTool;

public class HeatSink : ModuleRules
{
	public HeatSink(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",

			// AActor, UActorComponent and DrawDebugHelpers for the demo level.
			"Engine",

			// UHeatSinkSettings is a UDeveloperSettings, so the heat curve sits under
			// Project Settings > Plugins > HeatSink without an editor module.
			"DeveloperSettings",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});

		// Deliberately NOT here:
		//   GameplayAbilities - heat is a number and a lockout. Binding it to GAS would shut out
		//                       every project that fires its weapons another way, and the call
		//                       site is one line either way.
	}
}
