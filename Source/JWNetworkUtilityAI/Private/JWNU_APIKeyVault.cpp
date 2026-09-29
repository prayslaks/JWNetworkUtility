// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_APIKeyVault.h"

#include "JWNU_APIKeyStore.h"
#include "JWNU_KeyCrypto.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/ScopeExit.h"

void FJWNU_APIKeyVault::Clear(TArray<FEntry>& Entries)
{
	for (FEntry& Entry : Entries) FJWNU_KeyCrypto::ClearSensitiveString(Entry.Secret);
	Entries.Empty();
}

bool FJWNU_APIKeyVault::Load(const FString& AppId, const FString& Group, TArray<FEntry>& Entries, FGuid& Active)
{
	check(IsInGameThread());
	Clear(Entries); Active.Invalidate();
	if (!FJWNU_APIKeyStore::IsValidIdentifier(AppId) || !FJWNU_APIKeyStore::IsValidIdentifier(Group) || Group.Len() > 122) return false;
	const FString StorageName = TEXT("Vault-") + Group;
	if (!FJWNU_APIKeyStore::Exists(AppId, StorageName)) return true;
	FString Payload;
	ON_SCOPE_EXIT { FJWNU_KeyCrypto::ClearSensitiveString(Payload); };
	if (!FJWNU_APIKeyStore::Load(AppId, StorageName, Payload)) return false;
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Payload), Root) || !Root.IsValid()) return false;
	double Version = 0;
	FString ActiveString;
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (!Root->TryGetNumberField(TEXT("version"), Version) || Version != 1
		|| !Root->TryGetStringField(TEXT("active"), ActiveString)
		|| !Root->TryGetArrayField(TEXT("keys"), Values) || Values->Num() > 32) return false;
	if (!ActiveString.IsEmpty() && !FGuid::Parse(ActiveString, Active)) return false;
	TSet<FGuid> Ids;
	for (const TSharedPtr<FJsonValue>& Value : *Values)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object->IsValid()) return false;
		FEntry& Entry = Entries.AddDefaulted_GetRef();
		FString Id, Created, Updated, Expires;
		if (!(*Object)->TryGetStringField(TEXT("id"), Id) || !FGuid::Parse(Id, Entry.Info.Id) || !Entry.Info.Id.IsValid() || Ids.Contains(Entry.Info.Id)
			|| !(*Object)->TryGetStringField(TEXT("label"), Entry.Info.Label) || Entry.Info.Label.IsEmpty() || Entry.Info.Label.Len() > 64
			|| !(*Object)->TryGetStringField(TEXT("secret"), Entry.Secret) || Entry.Secret.IsEmpty() || Entry.Secret.Len() > 16384
			|| !(*Object)->TryGetStringField(TEXT("created"), Created)
			|| !(*Object)->TryGetStringField(TEXT("updated"), Updated)
			|| !(*Object)->TryGetStringField(TEXT("expires"), Expires)) return false;
		if ((!Created.IsEmpty() && !FDateTime::ParseIso8601(*Created, Entry.Info.CreatedUtc))
			|| (!Updated.IsEmpty() && !FDateTime::ParseIso8601(*Updated, Entry.Info.UpdatedUtc))
			|| (!Expires.IsEmpty() && !FDateTime::ParseIso8601(*Expires, Entry.Info.ExpiresUtc))) return false;
		Ids.Add(Entry.Info.Id);
	}
	return !Active.IsValid() || Ids.Contains(Active);
}

bool FJWNU_APIKeyVault::Save(const FString& AppId, const FString& Group, const TArray<FEntry>& Entries, const FGuid& Active)
{
	check(IsInGameThread());
	if (!FJWNU_APIKeyStore::IsValidIdentifier(AppId) || !FJWNU_APIKeyStore::IsValidIdentifier(Group) || Group.Len() > 122) return false;
	if (Entries.Num() > 32) return false;
	TSet<FGuid> Ids;
	for (const FEntry& Entry : Entries)
	{
		if (!Entry.Info.Id.IsValid() || Ids.Contains(Entry.Info.Id) || Entry.Info.Label.TrimStartAndEnd().IsEmpty()
			|| Entry.Info.Label.Len() > 64 || Entry.Secret.IsEmpty() || Entry.Secret.Len() > 16384) return false;
		for (TCHAR Char : Entry.Secret) if (Char == 0) return false;
		Ids.Add(Entry.Info.Id);
	}
	if (Active.IsValid() && !Ids.Contains(Active)) return false;
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("version"), 1);
	Root->SetStringField(TEXT("active"), Active.IsValid() ? Active.ToString() : FString());
	TArray<TSharedPtr<FJsonValue>> Values;
	for (const FEntry& Entry : Entries)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("id"), Entry.Info.Id.ToString());
		Object->SetStringField(TEXT("label"), Entry.Info.Label);
		Object->SetStringField(TEXT("secret"), Entry.Secret);
		Object->SetStringField(TEXT("created"), Entry.Info.CreatedUtc.GetTicks() ? Entry.Info.CreatedUtc.ToIso8601() : FString());
		Object->SetStringField(TEXT("updated"), Entry.Info.UpdatedUtc.GetTicks() ? Entry.Info.UpdatedUtc.ToIso8601() : FString());
		Object->SetStringField(TEXT("expires"), Entry.Info.ExpiresUtc.GetTicks() ? Entry.Info.ExpiresUtc.ToIso8601() : FString());
		Values.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("keys"), Values);
	FString Payload;
	ON_SCOPE_EXIT { FJWNU_KeyCrypto::ClearSensitiveString(Payload); };
	if (!FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Payload))) return false;
	return FJWNU_APIKeyStore::Save(AppId, TEXT("Vault-") + Group, Payload);
}

bool FJWNU_APIKeyVault::ListKeys(const FString& AppId, const FString& Group, TArray<FJWNU_APIKeyInfo>& OutKeys)
{
	OutKeys.Reset();
	TArray<FEntry> Entries; FGuid Active;
	ON_SCOPE_EXIT { Clear(Entries); };
	if (!Load(AppId, Group, Entries, Active)) return false;
	for (FEntry& Entry : Entries)
	{
		Entry.Info.bActive = Entry.Info.Id == Active;
		Entry.Info.MaskedKey = Entry.Secret.Len() > 8 ? TEXT("•••• ") + Entry.Secret.Right(4) : TEXT("••••••••");
		OutKeys.Add(Entry.Info);
	}
	return true;
}

bool FJWNU_APIKeyVault::SaveKey(const FString& AppId, const FString& Group, FGuid Id, const FString& Label, const FString& ApiKey, FDateTime ExpiresUtc, FGuid& OutId)
{
	OutId.Invalidate();
	FString Key = ApiKey.TrimStartAndEnd();
	ON_SCOPE_EXIT { FJWNU_KeyCrypto::ClearSensitiveString(Key); };
	if (Label.TrimStartAndEnd().IsEmpty() || Label.Len() > 64 || Key.Len() > 16384) return false;
	for (TCHAR Char : Key) if (Char == 0) return false;
	TArray<FEntry> Entries; FGuid Active;
	ON_SCOPE_EXIT { Clear(Entries); };
	if (!Load(AppId, Group, Entries, Active)) return false;
	FEntry* Entry = nullptr;
	if (!Id.IsValid())
	{
		if (Entries.Num() >= 32 || Key.IsEmpty()) return false;
		Entry = &Entries.AddDefaulted_GetRef();
		Entry->Info.Id = FGuid::NewGuid();
		Entry->Info.CreatedUtc = FDateTime::UtcNow();
		if (Entries.Num() == 1) Active = Entry->Info.Id;
	}
	else Entry = Entries.FindByPredicate([Id](const FEntry& Item) { return Item.Info.Id == Id; });
	if (!Entry) return false;
	if (!Key.IsEmpty())
	{
		FJWNU_KeyCrypto::ClearSensitiveString(Entry->Secret);
		Entry->Secret = Key;
	}
	Entry->Info.Label = Label.TrimStartAndEnd();
	Entry->Info.ExpiresUtc = ExpiresUtc;
	Entry->Info.UpdatedUtc = FDateTime::UtcNow();
	if (!Save(AppId, Group, Entries, Active)) return false;
	OutId = Entry->Info.Id;
	return true;
}

bool FJWNU_APIKeyVault::SelectKey(const FString& AppId, const FString& Group, FGuid Id)
{
	TArray<FEntry> Entries; FGuid Active;
	ON_SCOPE_EXIT { Clear(Entries); };
	if (!Load(AppId, Group, Entries, Active) || !Entries.ContainsByPredicate([Id](const FEntry& Entry) { return Entry.Info.Id == Id; })) return false;
	if (!Save(AppId, Group, Entries, Id)) return false;
	return true;
}

bool FJWNU_APIKeyVault::DeleteKey(const FString& AppId, const FString& Group, FGuid Id)
{
	TArray<FEntry> Entries; FGuid Active;
	ON_SCOPE_EXIT { Clear(Entries); };
	if (!Load(AppId, Group, Entries, Active)) return false;
	const int32 Index = Entries.IndexOfByPredicate([Id](const FEntry& Entry) { return Entry.Info.Id == Id; });
	if (Index == INDEX_NONE) return false;
	FJWNU_KeyCrypto::ClearSensitiveString(Entries[Index].Secret);
	Entries.RemoveAt(Index);
	const bool bDeletedActive = Active == Id;
	if (bDeletedActive) Active.Invalidate();
	if (!Save(AppId, Group, Entries, Active)) return false;
	return true;
}

bool FJWNU_APIKeyVault::DeleteAllKeys(const FString& AppId, const FString& Group)
{
	return Save(AppId, Group, {}, FGuid());
}

bool FJWNU_APIKeyVault::GetActiveKey(const FString& AppId, const FString& Group, FString& OutKey)
{
	FJWNU_KeyCrypto::ClearSensitiveString(OutKey);
	TArray<FEntry> Entries; FGuid Active;
	ON_SCOPE_EXIT { Clear(Entries); };
	if (!Load(AppId, Group, Entries, Active)) return false;
	const FEntry* Entry = Entries.FindByPredicate([Active](const FEntry& Item) { return Item.Info.Id == Active; });
	if (!Entry) return false;
	OutKey = Entry->Secret;
	return true;
}
