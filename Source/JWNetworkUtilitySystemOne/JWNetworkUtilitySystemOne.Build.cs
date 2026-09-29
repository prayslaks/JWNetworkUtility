// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class JWNetworkUtilitySystemOne : ModuleRules
{
    public JWNetworkUtilitySystemOne(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "JWNetworkUtility" });
        PrivateDependencyModuleNames.AddRange(new[] { "Json" });
    }
}
