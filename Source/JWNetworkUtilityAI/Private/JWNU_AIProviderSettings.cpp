// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_AIProviderSettings.h"

#include "JWNU_KeyCrypto.h"
#include "JWNU_APIKeyVault.h"
#include "Misc/App.h"

const TArray<FJWNU_AIProviderDefinition>& UJWNU_AIProviderSettings::GetProviderCatalog()
{
	// 공급자 ID는 저장소 식별자이므로 변경하지 않는다. 지원 종료 시 항목 대신 상태를 바꾼다.
	static const TArray<FJWNU_AIProviderDefinition> Catalog{{TEXT("OpenAI"), false}, {TEXT("Gemini"), false}, {TEXT("ElevenLabs"), false}, {TEXT("OpenRouter"), false}};
	return Catalog;
}

bool UJWNU_AIProviderSettings::IsKnownProvider(const FString& Provider)
{
	return GetProviderCatalog().ContainsByPredicate([&Provider](const FJWNU_AIProviderDefinition& Entry) { return Entry.Id == Provider; });
}

bool UJWNU_AIProviderSettings::IsSupportedProvider(const FString& Provider)
{
	return GetProviderCatalog().ContainsByPredicate([&Provider](const FJWNU_AIProviderDefinition& Entry) { return Entry.Id == Provider && !Entry.bDeprecated; });
}

FString UJWNU_AIProviderSettings::GetStorageNamespace() const
{
	return FApp::GetProjectName();
}

bool UJWNU_AIProviderSettings::GetSavedKeys(const FString& Provider, TArray<FJWNU_APIKeyInfo>& OutKeys) const
{
	OutKeys.Reset();
	if (!IsKnownProvider(Provider)) return false;
	const FString AppId = GetStorageNamespace();
	return FJWNU_APIKeyVault::ListKeys(AppId, Provider, OutKeys);
}

bool UJWNU_AIProviderSettings::SaveManagedKey(const FString& Provider, FGuid Id, const FString& Label, const FString& Key, FDateTime Expiry, FGuid& OutId) const
{
	OutId.Invalidate();
	if (!IsSupportedProvider(Provider)) return false;
	const FString AppId = GetStorageNamespace();
	return FJWNU_APIKeyVault::SaveKey(AppId, Provider, Id, Label, Key, Expiry, OutId);
}

bool UJWNU_AIProviderSettings::SelectManagedKey(const FString& Provider, FGuid Id) const
{
	return IsSupportedProvider(Provider) && FJWNU_APIKeyVault::SelectKey(GetStorageNamespace(), Provider, Id);
}

bool UJWNU_AIProviderSettings::DeleteManagedKey(const FString& Provider, FGuid Id) const
{
	return IsKnownProvider(Provider) && FJWNU_APIKeyVault::DeleteKey(GetStorageNamespace(), Provider, Id);
}

bool UJWNU_AIProviderSettings::GetApiKey(const FString& Provider, FString& OutKey) const
{
	FJWNU_KeyCrypto::ClearSensitiveString(OutKey);
	if (!IsSupportedProvider(Provider)) return false;
	const FString AppId = GetStorageNamespace();
	return FJWNU_APIKeyVault::GetActiveKey(AppId, Provider, OutKey);
}
