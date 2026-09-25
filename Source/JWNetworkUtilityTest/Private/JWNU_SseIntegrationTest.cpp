// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_SseTestReceiver.h"
#include "JWNU_BFL_SseClient.h"
#include "JWNU_BFL_ApiClientService.h"
#include "JWNU_GIS_ApiHostProvider.h"
#include "JWNU_GIS_ApiIdentityProvider.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#include "UObject/GarbageCollection.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_BreakStruct.h"

namespace JWNU::SseTest
{
/** 메모리에서 BP 이벤트→JSON wildcard 변환→구조체 소비 그래프를 컴파일한다. */
inline UBlueprint* BuildReceiverBlueprint(FAutomationTestBase* Test)
{
	UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_SseTestReceiver::StaticClass(), GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_SseAutomation")), BPTYPE_Normal,
		UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("SseEventGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
	const auto* Schema = GetDefault<UEdGraphSchema_K2>();
	auto* Event = NewObject<UK2Node_Event>(Graph);
	Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UJWNU_SseTestReceiver, ReceiveEvent), UJWNU_SseTestReceiver::StaticClass());
	Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
	auto* Break = NewObject<UK2Node_BreakStruct>(Graph);
	Break->StructType = FJWNU_SseEvent::StaticStruct(); Graph->AddNode(Break); Break->CreateNewGuid(); Break->AllocateDefaultPins();
	auto* Convert = NewObject<UK2Node_CallFunction>(Graph);
	Convert->SetFromFunction(UJWNU_BFL_ApiClientService::StaticClass()->FindFunctionByName(TEXT("ConvertJsonStringToStruct")));
	Graph->AddNode(Convert); Convert->CreateNewGuid(); Convert->AllocateDefaultPins();
	auto* Record = NewObject<UK2Node_CallFunction>(Graph);
	Record->SetFromFunction(UJWNU_SseTestReceiver::StaticClass()->FindFunctionByName(TEXT("RecordParsed")));
	Graph->AddNode(Record); Record->CreateNewGuid(); Record->AllocateDefaultPins();
	UEdGraphPin* StructInput = nullptr;
	for (auto* Pin : Break->Pins) { if (Pin->Direction == EGPD_Input) { StructInput = Pin; break; } }
	Test->TestTrue(TEXT("BP event struct wire"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Event")), StructInput));
	Test->TestTrue(TEXT("BP JSON input wire"), Schema->TryCreateConnection(Break->FindPinChecked(TEXT("Data")), Convert->FindPinChecked(TEXT("JsonString"))));
	Test->TestTrue(TEXT("BP wildcard USTRUCT wire"), Schema->TryCreateConnection(Convert->FindPinChecked(TEXT("OutStruct")), Record->FindPinChecked(TEXT("Payload"))));
	Test->TestTrue(TEXT("BP exec wire"), Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Convert->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
	Test->TestTrue(TEXT("BP record exec wire"), Schema->TryCreateConnection(Convert->FindPinChecked(UEdGraphSchema_K2::PN_Then), Record->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
	FKismetEditorUtilities::CompileBlueprint(BP);
	Test->TestTrue(TEXT("BP graph compiled"), BP->Status != BS_Error);
	return BP;
}

/** 실제 HTTP 요청을 여러 프레임에 걸쳐 검증하는 명령이다. */
class FIntegrationCommand : public IAutomationLatentCommand
{
public:
	explicit FIntegrationCommand(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		if (!Instance) { Setup(); }
		if (Stage >= 18) { Cleanup(); return true; }
		if (!Receiver) { StartStage(); }
		if (Stage == 0 && Receiver->EventCount > 0 && !bCollectedDuringStream)
		{
			bCollectedDuringStream = true;
			CollectGarbage(RF_NoFlags);
		}
		if ((Stage == 12 || Stage == 17) && Handle && Handle->IsRunning()
			&& Handle->GetJob() && !Handle->GetJob()->IsRunning())
		{
			if (Stage == 12 && !bCollectedDuringRefresh) { bCollectedDuringRefresh = true; CollectGarbage(RF_NoFlags); }
			if (Stage == 17) { Handle->Cancel(); }
		}
		if ((Stage == 5 || Stage == 6) && Receiver->EventCount > 0 && !Handle->IsCancelled())
		{
			if (Stage == 6) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); }
			else { Handle->Cancel(); Handle->Cancel(); }
		}
		if (Receiver->TerminalCount == 0 && FPlatformTime::Seconds() - StageStarted < 12) { return false; }
		if (FinishedAt == 0) { FinishedAt = FPlatformTime::Seconds(); return false; }
		// 종료 이후 늦은 콜백이나 이중 통지를 검증한다.
		if (FPlatformTime::Seconds() - FinishedAt < (Stage == 17 ? .6 : .15)) { return false; }
		ValidateStage();
		Receiver->RemoveFromRoot(); Receiver = nullptr; Handle = nullptr; FinishedAt = 0; ++Stage;
		return false;
	}
private:
	void Setup()
	{
		BaseURL = TEXT("http://127.0.0.1:18573");
		FParse::Value(FCommandLine::Get(), TEXT("JWNUSseTestURL="), BaseURL);
		Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone(); World = Instance->GetWorld();
		auto* Hosts = Instance->GetSubsystem<UJWNU_GIS_ApiHostProvider>();
		auto* Property = FindFProperty<FMapProperty>(Hosts->GetClass(), TEXT("ServiceTypeToHostMap"));
		auto* Map = Property->ContainerPtrToValuePtr<TMap<EJWNU_ServiceType, FString>>(Hosts);
		Map->Add(EJWNU_ServiceType::GameServer, BaseURL); Map->Add(EJWNU_ServiceType::AuthServer, BaseURL);
		Blueprint = BuildReceiverBlueprint(Test); Blueprint->AddToRoot();
		TokenPath = FPaths::ProjectSavedDir() / TEXT("Config/JWNetworkUtility") /
			FString::Printf(TEXT("auth_%s.bin"), *UEnum::GetDisplayValueAsText(EJWNU_ServiceType::GameServer).ToString());
		bHadTokenFile = FFileHelper::LoadFileToArray(TokenBackup, *TokenPath);
	}
	void StartStage()
	{
		Receiver = NewObject<UJWNU_SseTestReceiver>(GetTransientPackage(), Stage == 0 || Stage == 1 ? Blueprint->GeneratedClass.Get() : UJWNU_SseTestReceiver::StaticClass());
		Receiver->AddToRoot(); StageStarted = FPlatformTime::Seconds(); ParsedErrors = 0;
		Test->AddInfo(FString::Printf(TEXT("SSE integration stage %d"), Stage));
		FJWNU_SseOptions Options; Options.Headers.Add(TEXT("X-JWNU-Test"), TEXT("echo-header"));
		FString Path = TEXT("/sse/events?split=7");
		if (Stage == 3) { Path = TEXT("/sse/events?status=429&mode=large_error"); Options.MaxErrorBodyBytes = 64; }
		if (Stage == 4) { Path = TEXT("/sse/events?mode=wrong_type"); }
		if (Stage == 5 || Stage == 6) { Path = TEXT("/sse/events?count=10&delay=0.2"); }
		if (Stage == 7) { Path = TEXT("/sse/events?mode=stall"); Options.IdleTimeoutSeconds = .2f; }
		if (Stage == 8) { Path = TEXT("/sse/events?mode=disconnect"); }
		if (Stage == 9) { Path = TEXT("/sse/events?mode=unfinished"); }
		if (Stage == 10) { Path = TEXT("/sse/events?mode=bad_json"); }
		if (Stage == 11) { Path = TEXT("/sse/events?mode=before_open"); Options.OpenTimeoutSeconds = .2f; }
		if (Stage == 15) { Options.MaxQueuedBytes = 16; }
		if (Stage == 16) { Path = TEXT("/sse/events?count=20&delay=0.05"); Options.TotalTimeoutSeconds = .3f; }
		FJWNU_OnSseResponseBP Open; Open.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveOpened);
		FJWNU_OnSseEventBP Event; Event.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveEvent);
		FJWNU_OnSseResponseBP Complete; Complete.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveCompleted);
		FJWNU_OnSseResponseBP Error; Error.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveError);
		FJWNU_OnSseCancelledBP Cancel; Cancel.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveCancelled);
		FJWNU_SseCallbacks Callbacks;
		Callbacks.OnOpened.BindUObject(Receiver, &UJWNU_SseTestReceiver::ReceiveOpened);
		Callbacks.OnCompleted.BindUObject(Receiver, &UJWNU_SseTestReceiver::ReceiveCompleted);
		Callbacks.OnError.BindUObject(Receiver, &UJWNU_SseTestReceiver::ReceiveError);
		Callbacks.OnCancelled.BindUObject(Receiver, &UJWNU_SseTestReceiver::ReceiveCancelled);
		Callbacks.OnEvent.BindUObject(Receiver, &UJWNU_SseTestReceiver::ReceiveEvent);
		if (Stage == 0)
		{
			Handle = UJWNU_BFL_SseClient::SendSseRequest(World, EJWNU_HttpMethod::Post, TEXT("{\"test\":true}"), {}, Options, Open, Event, Complete, Error, Cancel, BaseURL + Path, TEXT(""));
			CollectGarbage(RF_NoFlags);
		}
		else if (Stage == 1)
		{
			Handle = UJWNU_BFL_SseClient::CallSseApi(World, EJWNU_HttpMethod::Get, TEXT(""), {}, Options, Open, Event, Complete, Error, Cancel, EJWNU_ServiceType::GameServer, Path, false);
		}
		else if (Stage == 2 || Stage == 10)
		{
			Handle = UJWNU_GIS_SseClient::CallSseApi_Template<FJWNU_SseTestPayload>(World, EJWNU_HttpMethod::Get, EJWNU_ServiceType::GameServer, Path, TEXT(""), {}, Options, Callbacks,
				[this](const FJWNU_SseEvent&, const FJWNU_SseTestPayload& Data) { Receiver->RecordParsed(Data); },
				[this](const FJWNU_SseEvent&, const FString&) { ++ParsedErrors; }, false);
		}
		else if (Stage == 12 || Stage == 17)
		{
			// 기존 일반 HTTP 경로로 로컬 테스트 인증 세션을 받고, 고의 401 이후 SSE 재개를 검사한다.
			FOnHttpResponseBPEvent Response;
			const auto Callback = FOnHttpRequestCompletedDelegate::CreateLambda([this, Options, Callbacks](int32 Status, const FString& Body)
			{
				FJWNU_RES_AuthRefresh Tokens;
				if (Status != 200 || !FJsonObjectConverter::JsonObjectStringToUStruct(Body, &Tokens, 0, 0)) { Receiver->TerminalCount++; Test->AddError(TEXT("Cannot get local SSE auth fixture")); return; }
				auto* Identity = Instance->GetSubsystem<UJWNU_GIS_ApiIdentityProvider>();
				Identity->SetAccessTokenContainer(EJWNU_ServiceType::GameServer, {Tokens.AccessToken, Tokens.ExpiresAt});
				Identity->SetRefreshTokenContainer(EJWNU_ServiceType::GameServer, {Tokens.RefreshToken, Tokens.RefreshTokenExpiresAt});
				Identity->SetUserId(Tokens.UserId);
				Handle = UJWNU_GIS_SseClient::CallSseApi_NoTemplate(World, EJWNU_HttpMethod::Get, EJWNU_ServiceType::GameServer, TEXT("/api/sse/events"), TEXT(""), {}, Options, Callbacks, true);
			});
			if (BootstrapJob) { BootstrapJob->RemoveFromRoot(); }
			BootstrapJob = UJWNU_GIS_HttpClientHelper::SendRequest_RawResponse(World, EJWNU_HttpMethod::Post, BaseURL + TEXT("/sse/test-session"), TEXT(""), TEXT(""), {}, Callback);
			BootstrapJob->AddToRoot();
		}
		else if (Stage == 13)
		{
			Handle = UJWNU_GIS_SseClient::SendSseRequest(World, EJWNU_HttpMethod::Get, BaseURL + Path, TEXT(""), TEXT(""), {}, Options, Callbacks);
			Handle->Cancel();
		}
		else if (Stage == 14)
		{
			Options.MaxEventBytes = 16;
			Handle = UJWNU_GIS_SseClient::SendSseRequest(World, EJWNU_HttpMethod::Get, BaseURL + Path, TEXT(""), TEXT(""), {}, Options, Callbacks);
		}
		else { Handle = UJWNU_GIS_SseClient::SendSseRequest(World, EJWNU_HttpMethod::Get, BaseURL + Path, TEXT(""), TEXT(""), {}, Options, Callbacks); }
	}
	void ValidateStage()
	{
		Test->TestEqual(TEXT("Exactly one terminal callback"), Receiver->TerminalCount, 1);
		if (Handle) { Test->TestFalse(TEXT("Handle finished"), Handle->IsRunning()); }
		if (Stage == 3)
		{
			Test->TestEqual(TEXT("429 preserved"), Receiver->LastResponse.StatusCode, 429);
			Test->TestEqual(TEXT("Retry-After preserved"), Receiver->LastResponse.Headers.FindRef(TEXT("retry-after")), FString(TEXT("1")));
			Test->TestTrue(TEXT("Bounded error body"), Receiver->LastResponse.bErrorBodyTruncated);
			Test->TestEqual(TEXT("No open on HTTP error"), Receiver->OpenCount, 0);
		}
		else if (Stage == 4) { Test->TestEqual(TEXT("Wrong MIME"), Receiver->LastResponse.Error, EJWNU_SseError::InvalidContentType); }
		else if (Stage == 5 || Stage == 6 || Stage == 13 || Stage == 17)
		{
			Test->TestTrue(TEXT("Cancellation signalled"), Receiver->bCancelled);
			Test->TestEqual(TEXT("No late events"), Receiver->EventCount, Stage == 13 || Stage == 17 ? 0 : 1);
		}
		else if (Stage == 7 || Stage == 11 || Stage == 16) { Test->TestEqual(TEXT("Timeout signalled"), Receiver->LastResponse.Error, EJWNU_SseError::Timeout); }
		else if (Stage == 8) { Test->TestEqual(TEXT("Disconnect signalled"), Receiver->LastResponse.Error, EJWNU_SseError::Network); }
		else if (Stage == 14 || Stage == 15) { Test->TestEqual(TEXT("Buffer cap signalled"), Receiver->LastResponse.Error, EJWNU_SseError::BufferLimit); }
		else
		{
			Test->TestEqual(TEXT("Opened once"), Receiver->OpenCount, 1);
			Test->TestEqual(TEXT("Successful stream"), Receiver->LastResponse.Error, EJWNU_SseError::None);
			Test->TestEqual(TEXT("Parsed count"), Receiver->EventCount, Stage == 10 ? 2 : 3);
			Test->TestEqual(TEXT("Unicode preserved"), Receiver->LastPayload.Text, FString(TEXT("안녕 ✈")));
			Test->TestTrue(TEXT("Incremental delivery before EOF"), Receiver->CompletedSeconds - Receiver->FirstEventSeconds > .1);
			if (Stage == 0)
			{
				Test->TestEqual(TEXT("POST body echoed"), Receiver->LastPayload.Echo, FString(TEXT("{\"test\":true}")));
				Test->TestEqual(TEXT("Custom header echoed"), Receiver->LastPayload.Header, FString(TEXT("echo-header")));
			}
			if (Stage == 10) { Test->TestEqual(TEXT("Typed parse error retained separately"), ParsedErrors, 1); }
			if (Stage == 12) { Test->TestTrue(TEXT("GC exercised during refresh"), bCollectedDuringRefresh); }
		}
	}
	void Cleanup()
	{
		if (BootstrapJob) { BootstrapJob->RemoveFromRoot(); }
		Instance->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false); Instance->RemoveFromRoot();
		Blueprint->RemoveFromRoot();
		if (bHadTokenFile) { FFileHelper::SaveArrayToFile(TokenBackup, *TokenPath); }
		else { IFileManager::Get().Delete(*TokenPath); }
	}
	FAutomationTestBase* Test;
	UGameInstance* Instance = nullptr;
	UWorld* World = nullptr;
	UBlueprint* Blueprint = nullptr;
	UJWNU_SseTestReceiver* Receiver = nullptr;
	UJWNU_HttpRequestJobHandle* Handle = nullptr;
	UJWNU_HttpRequestJob* BootstrapJob = nullptr;
	FString BaseURL, TokenPath;
	TArray<uint8> TokenBackup;
	bool bHadTokenFile = false;
	bool bCollectedDuringStream = false;
	bool bCollectedDuringRefresh = false;
	int32 Stage = 0, ParsedErrors = 0;
	double StageStarted = 0, FinishedAt = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_SseIntegrationTest, "JWNetworkUtility.SSE.FastAPI", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_SseIntegrationTest::RunTest(const FString& Parameters)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUSseIntegration")))
	{
		AddInfo(TEXT("Integration skipped; start the FastAPI fixture and pass -JWNUSseIntegration.")); return true;
	}
	ADD_LATENT_AUTOMATION_COMMAND(JWNU::SseTest::FIntegrationCommand(this));
	return true;
}
#endif
