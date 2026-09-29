// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

using UnrealBuildTool;
public class JWNetworkUtilityAI : ModuleRules
{
	public JWNetworkUtilityAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "DeveloperSettings" });
		PrivateDependencyModuleNames.Add("Json");
		if (Target.Platform == UnrealTargetPlatform.Win64) PublicSystemLibraries.Add("Crypt32.lib");
	}
}
