// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

using UnrealBuildTool;

public class JWNetworkUtilityTest : ModuleRules
{
    public JWNetworkUtilityTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Projects",						// IPluginManager
                "HTTP",							// FHttpModule, IHttpRequest, IHttpResponse
                "Json",							// FJsonObject, FJsonValue
                "JsonUtilities",				// FJsonObjectConverter, FJsonSerializer
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "UMG",
                "Slate",
                "SlateCore", 
                "JWNetworkUtility"
            }
        );
    }
}