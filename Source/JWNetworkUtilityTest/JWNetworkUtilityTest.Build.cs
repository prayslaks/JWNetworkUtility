// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

using UnrealBuildTool;

public class JWNetworkUtilityTest : ModuleRules
{
    public JWNetworkUtilityTest(ReadOnlyTargetRules Target) : base(Target)
    {
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BlueprintGraph", "KismetCompiler" });
		}
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "JWNetworkUtilityAudio",
                "CoreUObject",
                "Engine",
                "UMG",
                "Projects",						// IPluginManager
                "HTTP",							// FHttpModule, IHttpRequest, IHttpResponse
                "Json",							// FJsonObject, FJsonValue
                "JsonUtilities",				// FJsonObjectConverter, FJsonSerializer
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore", 
                "JWNetworkUtility",
                "JWNetworkUtilityOpenAI",
                "JWNetworkUtilityTypeSafe"
            }
        );
    }
}
