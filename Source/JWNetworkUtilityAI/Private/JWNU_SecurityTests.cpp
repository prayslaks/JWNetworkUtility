// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWNU_APIKeyStore.h"
#include "JWNU_KeyCrypto.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_PlatformCryptoTest,
	"JWNetworkUtility.AI.PlatformCrypto", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_PlatformCryptoTest::RunTest(const FString& Parameters)
{
	const TArray<uint8> Plain{0, 1, 2, 3, 255, 0, 9};
	TArray<uint8> Encrypted, Restored;
	TestTrue(TEXT("Windows protection is supported"), FJWNU_KeyCrypto::IsProtectionSupported());
	if (!TestTrue(TEXT("Protect binary data"), FJWNU_KeyCrypto::ProtectData(Plain, TEXT("test-context"), Encrypted))) return false;
	TestTrue(TEXT("Ciphertext differs from plaintext"), Encrypted != Plain);
	TestTrue(TEXT("Unprotect binary data"), FJWNU_KeyCrypto::UnprotectData(Encrypted, TEXT("test-context"), Restored));
	TestTrue(TEXT("Embedded zero bytes survive"), Plain == Restored);
	TestFalse(TEXT("Wrong context cannot decrypt"), FJWNU_KeyCrypto::UnprotectData(Encrypted, TEXT("wrong-context"), Restored));
	TestTrue(TEXT("Failed decryption clears output"), Restored.IsEmpty());
	Encrypted[Encrypted.Num() / 2] ^= 1;
	TestFalse(TEXT("Tampering is rejected"), FJWNU_KeyCrypto::UnprotectData(Encrypted, TEXT("test-context"), Restored));
	TestFalse(TEXT("Empty input is rejected"), FJWNU_KeyCrypto::ProtectData({}, TEXT("test-context"), Encrypted));
	TestTrue(TEXT("Failed protection clears output"), Encrypted.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_APIKeyStoreTest,
	"JWNetworkUtility.AI.APIKeyStore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_APIKeyStoreTest::RunTest(const FString& Parameters)
{
	const FString App = TEXT("JWNU-Test-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Name = TEXT("Provider");
	const FString Path = FJWNU_APIKeyStore::GetStoragePath(App, Name);
	ON_SCOPE_EXIT { FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, false); FJWNU_APIKeyStore::Delete(App, Name); };
	FString Read = TEXT("stale-output");
	TestFalse(TEXT("Missing key returns false"), FJWNU_APIKeyStore::Load(App, Name, Read));
	TestTrue(TEXT("Missing key clears output"), Read.IsEmpty());
	TestFalse(TEXT("Path traversal is rejected"), FJWNU_APIKeyStore::Save(TEXT("../escape"), Name, TEXT("fake")));
	if (!TestTrue(TEXT("Remembered key saved"), FJWNU_APIKeyStore::Save(App, Name, TEXT("  fake-saved-한글  ")))) return false;
	TestTrue(TEXT("Stored key is readable"), FJWNU_APIKeyStore::Load(App, Name, Read));
	TestEqual(TEXT("UTF8 key roundtrip trims outer whitespace"), Read, FString(TEXT("fake-saved-한글")));
	TestFalse(TEXT("Other app cannot read this key"), FJWNU_APIKeyStore::Load(App + TEXT("-other"), Name, Read));
	TArray<uint8> Raw;
	FFileHelper::LoadFileToArray(Raw, *Path);
	const FTCHARToUTF8 Plain(TEXT("fake-saved-한글"));
	bool bContainsPlain = false;
	for (int32 Index = 0; Index + Plain.Length() <= Raw.Num(); ++Index)
		bContainsPlain |= FMemory::Memcmp(Raw.GetData() + Index, Plain.Get(), Plain.Length()) == 0;
	TestFalse(TEXT("Stored bytes do not contain the plaintext"), bContainsPlain);
	TestTrue(TEXT("Read-only fixture is established"), FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, true));
	TestFalse(TEXT("Failed replacement is reported"), FJWNU_APIKeyStore::Save(App, Name, TEXT("fake-replacement")));
	FJWNU_APIKeyStore::Load(App, Name, Read);
	TestEqual(TEXT("Failed replacement preserves original"), Read, FString(TEXT("fake-saved-한글")));
	FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, false);
	TestTrue(TEXT("Successful replacement works"), FJWNU_APIKeyStore::Save(App, Name, TEXT("fake-new")));
	FJWNU_APIKeyStore::Load(App, Name, Read);
	TestEqual(TEXT("Replacement survives reload"), Read, FString(TEXT("fake-new")));
	FFileHelper::SaveArrayToFile(TArray<uint8>{1, 2, 3}, *Path);
	TestTrue(TEXT("Corrupt file is distinguished from absence"), FJWNU_APIKeyStore::Exists(App, Name));
	TestFalse(TEXT("Corrupt file fails closed"), FJWNU_APIKeyStore::Load(App, Name, Read));
	TestTrue(TEXT("Corruption returns no stale key"), Read.IsEmpty());
	TestTrue(TEXT("Forget removes corrupt file"), FJWNU_APIKeyStore::Delete(App, Name));
	TestFalse(TEXT("Forget leaves no file"), FJWNU_APIKeyStore::Exists(App, Name));
	TestTrue(TEXT("Forget missing key is idempotent"), FJWNU_APIKeyStore::Delete(App, Name));
	return true;
}

#endif
