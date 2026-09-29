// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

using UnrealBuildTool;
public class JWNetworkUtilityEditor : ModuleRules
{
	public JWNetworkUtilityEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "JWNetworkUtilityAI", "DeveloperSettings", "Slate", "SlateCore", "PropertyEditor", "UnrealEd", "InputCore" });
	}
}
