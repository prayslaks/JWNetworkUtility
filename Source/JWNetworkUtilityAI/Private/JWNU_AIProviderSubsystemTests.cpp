// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWNU_AIProviderSubsystem.h"
#include "JWNU_AIProviderSettings.h"
#include "JWNU_APIKeyStore.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_AIProviderSubsystemTest,
	"JWNetworkUtility.AI.AIProviderSubsystem", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_AIProviderSubsystemTest::RunTest(const FString& Parameters)
{
	const FString App = TEXT("JWNU-GISTest-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Provider = TEXT("OpenRouter");
	UGameInstance* FirstGame = NewObject<UGameInstance>();
	UGameInstance* SecondGame = NewObject<UGameInstance>();
	UJWNU_AIProviderSubsystem* First = NewObject<UJWNU_AIProviderSubsystem>(FirstGame);
	UJWNU_AIProviderSubsystem* Second = NewObject<UJWNU_AIProviderSubsystem>(SecondGame);
	First->StorageNamespace = App;
	Second->StorageNamespace = App;
	ON_SCOPE_EXIT
	{
		First->Deinitialize(); Second->Deinitialize();
		FJWNU_APIKeyStore::Delete(App, TEXT("Vault-CustomProvider"));
		FJWNU_APIKeyStore::Delete(App, Provider);
		FJWNU_APIKeyStore::Delete(App, TEXT("Vault-") + Provider);
	};
	TestEqual(TEXT("Subsystem belongs to owning GameInstance"), First->GetGameInstance(), FirstGame);
	TestEqual(TEXT("Settings are under JWNetworkUtility"), GetDefault<UJWNU_AIProviderSettings>()->GetCategoryName(), FName(TEXT("JWNetworkUtility")));
	TestTrue(TEXT("Blueprint API is reflected"), First->FindFunction(TEXT("SaveKey")) && First->FindFunction(TEXT("GetApiKey")) && First->FindFunction(TEXT("SelectKey")));
	TestNull(TEXT("Provider registry is not an editable property"), FindFProperty<FProperty>(UJWNU_AIProviderSettings::StaticClass(), TEXT("ProviderIds")));
	const TArray<FString> Supported = First->GetSupportedProviders();
	TestTrue(TEXT("Fixed supported providers are exposed to Blueprint"), Supported.Contains(TEXT("OpenAI")) && Supported.Contains(TEXT("Gemini")) && Supported.Contains(TEXT("ElevenLabs")) && Supported.Contains(TEXT("OpenRouter")));
	FGuid RejectedId;
	TestFalse(TEXT("Unregistered provider cannot save a key"), First->SaveKey(TEXT("CustomProvider"), {}, TEXT("unknown"), TEXT("fake"), {}, RejectedId));
	TestFalse(TEXT("Unregistered provider cannot create runtime override"), First->SetRuntimeApiKey(TEXT("CustomProvider"), TEXT("fake")));
	TArray<FJWNU_APIKeyInfo> UnknownKeys;
	TestFalse(TEXT("Unregistered provider cannot list keys"), First->ListKeys(TEXT("CustomProvider"), UnknownKeys));
	FGuid FirstId, SecondId;
	TestTrue(TEXT("GIS saves first persistent key"), First->SaveKey(Provider, {}, TEXT("first"), TEXT("fake-persistent-first"), {}, FirstId));
	TestTrue(TEXT("GIS saves second persistent key"), First->SaveKey(Provider, {}, TEXT("second"), TEXT("fake-persistent-second"), {}, SecondId));
	TArray<FJWNU_APIKeyInfo> Keys;
	TestTrue(TEXT("Other GameInstance sees shared saved list"), Second->ListKeys(Provider, Keys) && Keys.Num() == 2);
	TestTrue(TEXT("First instance runtime override accepted"), First->SetRuntimeApiKey(Provider, TEXT("fake-first-instance")));
	TestTrue(TEXT("Second instance runtime override accepted"), Second->SetRuntimeApiKey(Provider, TEXT("fake-second-instance")));
	FString Read;
	First->GetApiKey(Provider, Read);
	TestEqual(TEXT("First instance retains its own runtime key"), Read, FString(TEXT("fake-first-instance")));
	Second->GetApiKey(Provider, Read);
	TestEqual(TEXT("Second instance retains its own runtime key"), Read, FString(TEXT("fake-second-instance")));
	TestTrue(TEXT("Selecting saved key clears only caller override"), First->SelectKey(Provider, SecondId) && !First->HasRuntimeOverride(Provider));
	First->GetApiKey(Provider, Read);
	TestEqual(TEXT("Selected key reaches runtime consumer"), Read, FString(TEXT("fake-persistent-second")));
	TestTrue(TEXT("Other instance override survives selection"), Second->HasRuntimeOverride(Provider));
	Second->ClearRuntimeApiKey(Provider);
	Second->GetApiKey(Provider, Read);
	TestEqual(TEXT("Clearing runtime override uses latest shared selection"), Read, FString(TEXT("fake-persistent-second")));
	First->SetRuntimeApiKey(Provider, TEXT(""));
	TestFalse(TEXT("Empty runtime key masks saved key"), First->HasApiKey(Provider));
	TestTrue(TEXT("Other instance still reads saved key"), Second->HasApiKey(Provider));
	First->Deinitialize();
	TestTrue(TEXT("Deinitialize wipes in-memory overrides"), First->RuntimeKeys.IsEmpty());
	TestTrue(TEXT("Deinitialize does not remove persistent keys"), Second->HasApiKey(Provider));
	TestTrue(TEXT("GIS deletes selected key"), Second->DeleteKey(Provider, SecondId));
	TestFalse(TEXT("Selected-key deletion leaves no automatic replacement"), Second->HasApiKey(Provider));
	TestFalse(TEXT("Invalid provider cannot create runtime key"), Second->SetRuntimeApiKey(TEXT("../invalid"), TEXT("fake")));
	TestTrue(TEXT("GIS deletes all saved keys"), Second->DeleteAllKeys(Provider));
	TestTrue(TEXT("Deletion persists an empty list"), Second->ListKeys(Provider, Keys) && Keys.IsEmpty());
	return true;
}

#endif
