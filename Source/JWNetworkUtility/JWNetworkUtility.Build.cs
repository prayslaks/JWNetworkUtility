// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

using UnrealBuildTool;

public class JWNetworkUtility : ModuleRules
{
	public JWNetworkUtility(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			[
				// ... add public include paths required here ...
			]
		);
		
		PrivateIncludePaths.AddRange(
			[
				// ... add other private include paths required here ...
			]
		);
		
		PublicDependencyModuleNames.AddRange(
			[
				"Core",
				"Projects",						// IPluginManager
				"HTTP",							// FHttpModule, IHttpRequest, IHttpResponse
				"Json",							// FJsonObject, FJsonValue
				"JsonUtilities",				// FJsonObjectConverter, FJsonSerializer
				"OnlineSubsystem",
				"OnlineSubsystemSteam",
				"OnlineSubsystemUtils"
			]
		);
		
		PrivateDependencyModuleNames.AddRange(
			[
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			]
		);
		
		DynamicallyLoadedModuleNames.AddRange(
			[
				// ... add any modules that your module loads dynamically here ...
			]
		);
	}
}