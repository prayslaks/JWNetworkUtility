// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

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
