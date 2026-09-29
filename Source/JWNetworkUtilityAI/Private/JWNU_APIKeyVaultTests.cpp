// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWNU_APIKeyVault.h"
#include "JWNU_APIKeyStore.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_APIKeyVaultTest,
	"JWNetworkUtility.AI.APIKeyVault", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_APIKeyVaultTest::RunTest(const FString& Parameters)
{
	const FString App = TEXT("JWNU-VaultTest-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Group = TEXT("Provider");
	ON_SCOPE_EXIT { FJWNU_APIKeyStore::Delete(App, Group); FJWNU_APIKeyStore::Delete(App, TEXT("Vault-") + Group); };
	TArray<FJWNU_APIKeyInfo> Keys;
	TestFalse(TEXT("Invalid path identifier rejected on read"), FJWNU_APIKeyVault::ListKeys(TEXT("../bad"), Group, Keys));
	TestTrue(TEXT("New vault is empty"), FJWNU_APIKeyVault::ListKeys(App, Group, Keys) && Keys.IsEmpty());
	FGuid First, Second, Edited;
	const FDateTime Expired(2020, 1, 1);
	TestTrue(TEXT("First key saved"), FJWNU_APIKeyVault::SaveKey(App, Group, {}, TEXT("개발"), TEXT("fake-vault-first-1234"), {}, First));
	TestTrue(TEXT("Second key saved"), FJWNU_APIKeyVault::SaveKey(App, Group, {}, TEXT("테스트"), TEXT("fake-vault-second-5678"), Expired, Second));
	TestTrue(TEXT("List includes both keys"), FJWNU_APIKeyVault::ListKeys(App, Group, Keys) && Keys.Num() == 2);
	if (Keys.Num() != 2) return false;
	TestTrue(TEXT("Only first key starts active"), Keys[0].bActive && !Keys[1].bActive);
	TestTrue(TEXT("Display metadata masks full key"), Keys[0].MaskedKey.EndsWith(TEXT("1234")) && !Keys[0].MaskedKey.Contains(TEXT("fake-vault")));
	const FDateTime Created = Keys[1].CreatedUtc;
	TestTrue(TEXT("Registration time is recorded"), Created.GetTicks() > 0);
	TestTrue(TEXT("Select second key"), FJWNU_APIKeyVault::SelectKey(App, Group, Second));
	FString Read;
	TestTrue(TEXT("Expired management date does not block API access"), FJWNU_APIKeyVault::GetActiveKey(App, Group, Read));
	TestEqual(TEXT("Selection changes actual returned key"), Read, FString(TEXT("fake-vault-second-5678")));
	TestTrue(TEXT("Edit metadata without secret replacement"), FJWNU_APIKeyVault::SaveKey(App, Group, Second, TEXT("새 이름"), TEXT(""), {}, Edited));
	TestTrue(TEXT("Editing keeps ID"), Edited == Second);
	FJWNU_APIKeyVault::ListKeys(App, Group, Keys);
	TestTrue(TEXT("Editing preserves creation time and clears expiry"), Keys[1].CreatedUtc == Created && Keys[1].ExpiresUtc.GetTicks() == 0);
	FJWNU_APIKeyVault::GetActiveKey(App, Group, Read);
	TestEqual(TEXT("Empty edit preserves secret"), Read, FString(TEXT("fake-vault-second-5678")));
	TestTrue(TEXT("Delete active key"), FJWNU_APIKeyVault::DeleteKey(App, Group, Second));
	TestFalse(TEXT("Deleting active key does not select another"), FJWNU_APIKeyVault::GetActiveKey(App, Group, Read));
	TestFalse(TEXT("Unknown ID cannot be edited"), FJWNU_APIKeyVault::SaveKey(App, Group, Second, TEXT("gone"), TEXT("fake"), {}, Edited));
	TestFalse(TEXT("Unknown ID cannot be selected"), FJWNU_APIKeyVault::SelectKey(App, Group, Second));
	TestTrue(TEXT("Delete all persists empty vault"), FJWNU_APIKeyVault::DeleteAllKeys(App, Group));
	TestTrue(TEXT("Delete all leaves empty list"), FJWNU_APIKeyVault::ListKeys(App, Group, Keys) && Keys.IsEmpty());
	FJWNU_APIKeyStore::Save(App, TEXT("Vault-") + Group, TEXT("invalid-json"));
	TestFalse(TEXT("Corrupt vault cannot be silently overwritten"), FJWNU_APIKeyVault::SaveKey(App, Group, {}, TEXT("new"), TEXT("fake"), {}, Edited));
	FJWNU_APIKeyStore::Load(App, TEXT("Vault-") + Group, Read);
	TestEqual(TEXT("Corrupt original is retained"), Read, FString(TEXT("invalid-json")));
	return true;
}

#endif
