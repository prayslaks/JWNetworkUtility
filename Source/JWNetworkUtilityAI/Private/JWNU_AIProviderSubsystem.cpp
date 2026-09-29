// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_AIProviderSubsystem.h"

#include "JWNU_AIProviderSettings.h"
#include "JWNU_KeyCrypto.h"
#include "JWNU_APIKeyVault.h"
#include "Misc/ScopeExit.h"

void UJWNU_AIProviderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StorageNamespace = GetDefault<UJWNU_AIProviderSettings>()->GetStorageNamespace();
}

void UJWNU_AIProviderSubsystem::ClearRuntimeKeys()
{
	for (auto& Pair : RuntimeKeys) FJWNU_KeyCrypto::ClearSensitiveString(Pair.Value);
	RuntimeKeys.Empty();
}

void UJWNU_AIProviderSubsystem::Deinitialize()
{
	ClearRuntimeKeys();
	Super::Deinitialize();
}

void UJWNU_AIProviderSubsystem::BeginDestroy()
{
	ClearRuntimeKeys();
	Super::BeginDestroy();
}

bool UJWNU_AIProviderSubsystem::PrepareProvider(const FString& Provider)
{
	check(IsInGameThread());
	if (StorageNamespace.IsEmpty()) StorageNamespace = GetDefault<UJWNU_AIProviderSettings>()->GetStorageNamespace();
	return UJWNU_AIProviderSettings::IsKnownProvider(Provider);
}

TArray<FString> UJWNU_AIProviderSubsystem::GetSupportedProviders() const
{
	TArray<FString> Providers;
	for (const FJWNU_AIProviderDefinition& Entry : UJWNU_AIProviderSettings::GetProviderCatalog())
		if (!Entry.bDeprecated) Providers.Add(Entry.Id);
	return Providers;
}

bool UJWNU_AIProviderSubsystem::ListKeys(const FString& Provider, TArray<FJWNU_APIKeyInfo>& OutKeys)
{
	OutKeys.Reset();
	return PrepareProvider(Provider) && FJWNU_APIKeyVault::ListKeys(StorageNamespace, Provider, OutKeys);
}

bool UJWNU_AIProviderSubsystem::SaveKey(const FString& Provider, FGuid Id, const FString& Label, const FString& ApiKey, FDateTime ExpiresUtc, FGuid& OutId)
{
	OutId.Invalidate();
	if (!UJWNU_AIProviderSettings::IsSupportedProvider(Provider) || !PrepareProvider(Provider) || !FJWNU_APIKeyVault::SaveKey(StorageNamespace, Provider, Id, Label, ApiKey, ExpiresUtc, OutId)) return false;
	TArray<FJWNU_APIKeyInfo> Keys;
	if (FJWNU_APIKeyVault::ListKeys(StorageNamespace, Provider, Keys)
		&& Keys.ContainsByPredicate([OutId](const FJWNU_APIKeyInfo& Info) { return Info.Id == OutId && Info.bActive; })) ClearRuntimeApiKey(Provider);
	return true;
}

bool UJWNU_AIProviderSubsystem::SelectKey(const FString& Provider, FGuid Id)
{
	if (!UJWNU_AIProviderSettings::IsSupportedProvider(Provider) || !PrepareProvider(Provider) || !FJWNU_APIKeyVault::SelectKey(StorageNamespace, Provider, Id)) return false;
	ClearRuntimeApiKey(Provider);
	return true;
}

bool UJWNU_AIProviderSubsystem::DeleteKey(const FString& Provider, FGuid Id)
{
	TArray<FJWNU_APIKeyInfo> Keys;
	if (!ListKeys(Provider, Keys)) return false;
	const bool bActive = Keys.ContainsByPredicate([Id](const FJWNU_APIKeyInfo& Info) { return Info.Id == Id && Info.bActive; });
	if (!FJWNU_APIKeyVault::DeleteKey(StorageNamespace, Provider, Id)) return false;
	if (bActive) ClearRuntimeApiKey(Provider);
	return true;
}

bool UJWNU_AIProviderSubsystem::DeleteAllKeys(const FString& Provider)
{
	if (!PrepareProvider(Provider) || !FJWNU_APIKeyVault::DeleteAllKeys(StorageNamespace, Provider)) return false;
	ClearRuntimeApiKey(Provider);
	return true;
}

bool UJWNU_AIProviderSubsystem::GetApiKey(const FString& Provider, FString& OutApiKey)
{
	check(IsInGameThread());
	FJWNU_KeyCrypto::ClearSensitiveString(OutApiKey);
	if (!UJWNU_AIProviderSettings::IsSupportedProvider(Provider)) return false;
	if (const FString* Key = RuntimeKeys.Find(Provider))
	{
		OutApiKey = *Key;
		return !OutApiKey.IsEmpty();
	}
	return PrepareProvider(Provider) && FJWNU_APIKeyVault::GetActiveKey(StorageNamespace, Provider, OutApiKey);
}

bool UJWNU_AIProviderSubsystem::HasApiKey(const FString& Provider)
{
	FString Key;
	ON_SCOPE_EXIT { FJWNU_KeyCrypto::ClearSensitiveString(Key); };
	return GetApiKey(Provider, Key);
}

bool UJWNU_AIProviderSubsystem::SetRuntimeApiKey(const FString& Provider, const FString& ApiKey)
{
	check(IsInGameThread());
	if (!UJWNU_AIProviderSettings::IsSupportedProvider(Provider) || ApiKey.Len() > 16384) return false;
	for (TCHAR Char : ApiKey) if (Char == 0) return false;
	ClearRuntimeApiKey(Provider);
	RuntimeKeys.Add(Provider, ApiKey.TrimStartAndEnd());
	return true;
}

void UJWNU_AIProviderSubsystem::ClearRuntimeApiKey(const FString& Provider)
{
	check(IsInGameThread());
	if (FString* Key = RuntimeKeys.Find(Provider)) FJWNU_KeyCrypto::ClearSensitiveString(*Key);
	RuntimeKeys.Remove(Provider);
}

bool UJWNU_AIProviderSubsystem::HasRuntimeOverride(const FString& Provider) const
{
	return RuntimeKeys.Contains(Provider);
}
