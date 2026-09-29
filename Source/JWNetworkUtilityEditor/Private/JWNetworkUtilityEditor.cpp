// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "JWNU_AIProviderKeysCustomization.h"

/** JWNU 공급자 키 관리 화면을 에디터 모듈 수명에 맞춰 등록·해제한다. */
class FJWNetworkUtilityEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& Editor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		Editor.RegisterCustomClassLayout(TEXT("JWNU_AIProviderSettings"), FOnGetDetailCustomizationInstance::CreateStatic(&FJWNU_AIProviderKeysCustomization::MakeInstance));
		Editor.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FPropertyEditorModule* Editor = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
			Editor->UnregisterCustomClassLayout(TEXT("JWNU_AIProviderSettings"));
	}
};

IMPLEMENT_MODULE(FJWNetworkUtilityEditorModule, JWNetworkUtilityEditor)
