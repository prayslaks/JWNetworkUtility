// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNetworkUtility.h"

#include "GenericPlatform/GenericPlatformHttp.h"

#define LOCTEXT_NAMESPACE "FJWNetworkUtilityModule"

void FJWNetworkUtilityModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FJWNetworkUtilityModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FJWNetworkUtilityModule, JWNetworkUtility)

DEFINE_LOG_CATEGORY(JWLog);
DEFINE_LOG_CATEGORY(JWLogWall);