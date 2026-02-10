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
                "OnlineSubsystem",
                "OnlineSubsystemSteam",
                "OnlineSubsystemUtils",
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