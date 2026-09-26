// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

using UnrealBuildTool;
public class JWNetworkUtilityAudio : ModuleRules
{
    public JWNetworkUtilityAudio(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        if (Target.Type != TargetType.Server)
        {
            PrivateDependencyModuleNames.AddRange(new[] { "AudioCaptureCore", "SignalProcessing", "AudioCaptureWasapi" });
        }
    }
}
