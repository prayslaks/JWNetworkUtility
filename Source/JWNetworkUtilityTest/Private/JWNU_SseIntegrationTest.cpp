// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_SseTestReceiver.h"
#include "JWNU_SseRequest.h"
#include "JWNU_BFL_SseClient.h"
#include "JWNU_HttpRequest.h"
#include "JWNU_GIS_ApiClientService.h"
#include "UObject/StrongObjectPtr.h"
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
	for (bool bService : {false, true})
	{
		auto* StartEvent = NewObject<UK2Node_Event>(Graph);
		StartEvent->EventReference.SetExternalMember(bService ? TEXT("StartServiceDefaults") : TEXT("StartDirectDefaults"), UJWNU_SseTestReceiver::StaticClass());
		StartEvent->bOverrideFunction = true; Graph->AddNode(StartEvent); StartEvent->CreateNewGuid(); StartEvent->AllocateDefaultPins();
		auto* StartCall = NewObject<UK2Node_CallFunction>(Graph);
		StartCall->SetFromFunction((bService ? UJWNU_SseApiRequest::StaticClass() : UJWNU_SseRequest::StaticClass())->FindFunctionByName(TEXT("Start")));
		Graph->AddNode(StartCall); StartCall->CreateNewGuid(); StartCall->AllocateDefaultPins();
		Test->TestTrue(TEXT("SSE Start exec"), Schema->TryCreateConnection(StartEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
		Test->TestTrue(TEXT("SSE Start target"), Schema->TryCreateConnection(StartEvent->FindPinChecked(TEXT("Target")), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Self)));
		Test->TestTrue(TEXT("SSE Start address"), Schema->TryCreateConnection(StartEvent->FindPinChecked(TEXT("Address")), StartCall->FindPinChecked(bService ? TEXT("Endpoint") : TEXT("URL"))));
		if (bService) { Schema->TrySetDefaultValue(*StartCall->FindPinChecked(TEXT("bRequiresAuth")), TEXT("false")); }
		Test->TestTrue(TEXT("SSE Options unconnected"), StartCall->FindPinChecked(TEXT("Options"))->LinkedTo.IsEmpty());
		Test->TestTrue(TEXT("SSE QueryParams unconnected"), StartCall->FindPinChecked(TEXT("QueryParams"))->LinkedTo.IsEmpty());
	}
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
	explicit FIntegrationCommand(FAutomationTestBase* InTest, bool bInImmediate = false) : Test(InTest), bImmediate(bInImmediate) {}
	virtual bool Update() override
	{
		if (!Instance) { Setup(); }
		if (Stage >= 21) { Cleanup(); return true; }
		if (!Receiver) { StartStage(); }
		if (Stage == 0 && Receiver->EventCount > 0 && !bCollectedDuringStream)
		{
			bCollectedDuringStream = true;
			CollectGarbage(RF_NoFlags);
		}
		auto* Service = Instance->GetSubsystem<UJWNU_GIS_ApiClientService>();
		const auto* RefreshJobs = Service ? FindFProperty<FMapProperty>(Service->GetClass(), TEXT("ActiveRefreshJobs"))
			->ContainerPtrToValuePtr<TMap<EJWNU_ServiceType, TObjectPtr<UJWNU_HttpRequestJob>>>(Service) : nullptr;
		if ((Stage == 12 || Stage == 17) && Request && Request->IsActive() && RefreshJobs && !RefreshJobs->IsEmpty())
		{
			if (Stage == 12 && !bCollectedDuringRefresh) { bCollectedDuringRefresh = true; CollectGarbage(RF_NoFlags); }
			if (Stage == 17) { Request->Cancel(); }
		}
		if ((Stage == 5 || Stage == 6) && Receiver->EventCount > 0 && Request->IsActive())
		{
			if (Stage == 6) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); }
			else { Request->Cancel(); Request->Cancel(); }
		}
		if (Receiver->TerminalCount == 0 && FPlatformTime::Seconds() - StageStarted < 12) { return false; }
		if (FinishedAt == 0) { FinishedAt = FPlatformTime::Seconds(); return false; }
		// 종료 이후 늦은 콜백이나 이중 통지를 검증한다.
		if (FPlatformTime::Seconds() - FinishedAt < (Stage == 17 ? .6 : .15)) { return false; }
		ValidateStage();
		Receiver->RemoveFromRoot(); Receiver = nullptr; Request = nullptr; FinishedRequest.Reset(); FinishedAt = 0; ++Stage;
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
		Receiver = NewObject<UJWNU_SseTestReceiver>(GetTransientPackage(), Stage == 0 || Stage == 1 || Stage == 18 || Stage == 19 ? Blueprint->GeneratedClass.Get() : UJWNU_SseTestReceiver::StaticClass());
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
		if (Stage == 12 || Stage == 17)
        {
            BootstrapRequest.Reset(UJWNU_HttpRequest::CreateHttpRequest(World));
            BootstrapRequest->OnCompletedNative.AddLambda([this, Options](const FJWNU_HttpResult& Response)
            {
                FJWNU_RES_AuthRefresh Tokens;
                if (!FJsonObjectConverter::JsonObjectStringToUStruct(Response.ResponseBody, &Tokens, 0, 0))
                { Receiver->TerminalCount++; Test->AddError(TEXT("Cannot get local SSE auth fixture")); return; }
                auto* Identity = Instance->GetSubsystem<UJWNU_GIS_ApiIdentityProvider>();
                Identity->SetAccessTokenContainer(EJWNU_ServiceType::GameServer, {Tokens.AccessToken, Tokens.ExpiresAt});
                Identity->SetRefreshTokenContainer(EJWNU_ServiceType::GameServer, {Tokens.RefreshToken, Tokens.RefreshTokenExpiresAt});
                Identity->SetUserId(Tokens.UserId);
                if (bImmediate) { CallImmediate(TEXT("/api/sse/events"), Options, true); }
                else
                {
                    auto* ServiceRequest = UJWNU_SseApiRequest::CreateSseApiRequest(World);
                    BindRequest(ServiceRequest);
                    Test->TestTrue(TEXT("Authenticated Start accepted"), ServiceRequest->Start(EJWNU_ServiceType::GameServer, EJWNU_HttpMethod::Get, TEXT("/api/sse/events"), TEXT(""), {}, Options, true));
                }
            });
            BootstrapRequest->OnFailedNative.AddLambda([this](const FJWNU_HttpError&)
            { Receiver->TerminalCount++; Test->AddError(TEXT("Cannot start local SSE auth fixture")); });
            BootstrapRequest->Start(EJWNU_HttpMethod::Post, BaseURL + TEXT("/sse/test-session"), TEXT(""), {});
        }
        else if (bImmediate)
        {
            if (Stage == 14) { Options.MaxEventBytes = 16; }
            CallImmediate(Path, Stage == 18 || Stage == 19 ? FJWNU_SseOptions() : Options, false);
            if (Stage == 13) { Request->Cancel(); }
            if (Stage == 20) { Instance->Shutdown(); bShutdown = true; }
            if (Stage == 0) { CollectGarbage(RF_NoFlags); }
        }
        else if (Stage == 1 || Stage == 2 || Stage == 10 || Stage == 19)
        {
            auto* ServiceRequest = UJWNU_SseApiRequest::CreateSseApiRequest(World);
            BindRequest(ServiceRequest);
            if (Stage == 19) { Receiver->StartServiceDefaults(ServiceRequest, Path); }
            else { Test->TestTrue(TEXT("Service Start accepted"), ServiceRequest->Start(EJWNU_ServiceType::GameServer, EJWNU_HttpMethod::Get, Path, TEXT(""), {}, Options, false)); }
        }
        else
        {
            auto* DirectRequest = UJWNU_SseRequest::CreateSseRequest(World);
            BindRequest(DirectRequest);
            if (Stage == 14) { Options.MaxEventBytes = 16; }
            if (Stage == 18) { Receiver->StartDirectDefaults(DirectRequest, BaseURL + Path); }
            else { Test->TestTrue(TEXT("Direct Start accepted"), DirectRequest->Start(Stage == 0 ? EJWNU_HttpMethod::Post : EJWNU_HttpMethod::Get,
                BaseURL + Path, Stage == 0 ? TEXT("{\"test\":true}") : TEXT(""), {}, Options)); }
            if (Stage == 13) { Request->Cancel(); }
            if (Stage == 20) { Instance->Shutdown(); bShutdown = true; }
            if (Stage == 0) { CollectGarbage(RF_NoFlags); }
        }
    }
    void CallImmediate(const FString& Path, const FJWNU_SseOptions& Options, bool bRequiresAuth)
    {
        FJWNU_OnSseResponseBP Opened, Completed, Error;
        FJWNU_OnSseEventBP Event;
        FJWNU_OnSseCancelledBP Cancelled;
        Opened.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveOpened);
        Completed.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveCompleted);
        Error.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveError);
        Event.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveEvent);
        Cancelled.BindDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveCancelled);
        Request = UJWNU_BFL_SseClient::CallSseApi(World, Stage == 0 ? EJWNU_HttpMethod::Post : EJWNU_HttpMethod::Get,
            Stage == 0 ? TEXT("{\"test\":true}") : TEXT(""), {}, Options, Opened, Event, Completed, Error, Cancelled,
            EJWNU_ServiceType::GameServer, Path, bRequiresAuth);
        Test->TestNotNull(TEXT("Immediate SSE returns control request"), Request);
        if (!Request->IsActive()) { FinishedRequest.Reset(Request); }
        Request->OnCompletedNative.AddLambda([this](const FJWNU_SseResponse&) { FinishedRequest.Reset(Request); });
        Request->OnFailedNative.AddLambda([this](const FJWNU_SseResponse&) { FinishedRequest.Reset(Request); });
    }
    void BindRequest(UJWNU_SseRequestBase* InRequest)
    {
        Request = InRequest;
        Test->TestNotNull(TEXT("Factory creates SSE request"), Request);
        Test->TestFalse(TEXT("Factory does not start transmission"), Request->IsActive());
        Request->OnOpened.AddDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveOpened);
        Request->OnCompleted.AddDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveCompleted);
        Request->OnFailed.AddDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveError);
        Request->OnCompletedNative.AddLambda([this](const FJWNU_SseResponse&) { FinishedRequest.Reset(Request); });
        Request->OnFailedNative.AddLambda([this](const FJWNU_SseResponse&) { FinishedRequest.Reset(Request); });
        if (Stage == 2 || Stage == 10)
        {
            Request->OnEventNative.AddLambda([this](const FJWNU_SseEvent& Event)
            {
                FJWNU_SseTestPayload Payload;
                if (FJsonObjectConverter::JsonObjectStringToUStruct(Event.Data, &Payload, 0, 0)) { Receiver->RecordParsed(Payload); }
                else { ++ParsedErrors; }
            });
        }
        else { Request->OnEvent.AddDynamic(Receiver, &UJWNU_SseTestReceiver::ReceiveEvent); }
    }
	void ValidateStage()
	{
		Test->TestEqual(TEXT("Exactly one terminal callback"), Receiver->TerminalCount, 1);
		if (Request) { Test->TestFalse(TEXT("Request finished"), Request->IsActive()); }
		if (Request) { Test->TestEqual(TEXT("SSE common terminal state"), Request->GetState(), Receiver->bCancelled ? EJWNU_RequestState::Cancelled :
            (Receiver->LastResponse.Error == EJWNU_SseError::None ? EJWNU_RequestState::Succeeded : EJWNU_RequestState::Failed)); }
		if (auto* DirectRequest = Cast<UJWNU_SseRequest>(Request))
		{ Test->TestFalse(TEXT("Direct request is single use"), DirectRequest->Start(EJWNU_HttpMethod::Get, BaseURL + TEXT("/sse/events"), TEXT(""), {}, {})); }
		if (auto* ServiceRequest = Cast<UJWNU_SseApiRequest>(Request))
		{ Test->TestFalse(TEXT("Service request is single use"), ServiceRequest->Start(EJWNU_ServiceType::GameServer, EJWNU_HttpMethod::Get, TEXT("/sse/events"), TEXT(""), {}, {}, false)); }
		if (Stage == 3)
		{
			Test->TestEqual(TEXT("429 preserved"), Receiver->LastResponse.StatusCode, 429);
			Test->TestEqual(TEXT("Retry-After preserved"), Receiver->LastResponse.Headers.FindRef(TEXT("retry-after")), FString(TEXT("1")));
			Test->TestTrue(TEXT("Bounded error body"), Receiver->LastResponse.bErrorBodyTruncated);
			Test->TestEqual(TEXT("No open on HTTP error"), Receiver->OpenCount, 0);
		}
		else if (Stage == 4) { Test->TestEqual(TEXT("Wrong MIME"), Receiver->LastResponse.Error, EJWNU_SseError::InvalidContentType); }
		else if (Stage == 5 || Stage == 6 || Stage == 13 || Stage == 17 || Stage == 20)
		{
			Test->TestTrue(TEXT("Cancellation signalled"), Receiver->bCancelled);
			Test->TestEqual(TEXT("No late events"), Receiver->EventCount, Stage == 13 || Stage == 17 || Stage == 20 ? 0 : 1);
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
			if (Stage == 10) { Test->TestEqual(TEXT("Typed parse error retained separately"), bImmediate ? Receiver->ParseErrorCount : ParsedErrors, 1); }
			if (Stage == 12) { Test->TestTrue(TEXT("GC exercised during refresh"), bCollectedDuringRefresh); }
		}
	}
	void Cleanup()
	{
		if (BootstrapRequest.IsValid()) { BootstrapRequest->Cancel(); BootstrapRequest.Reset(); }
		if (!bShutdown) { Instance->Shutdown(); } GEngine->DestroyWorldContext(World); World->DestroyWorld(false); Instance->RemoveFromRoot();
		Blueprint->RemoveFromRoot();
		if (bHadTokenFile) { FFileHelper::SaveArrayToFile(TokenBackup, *TokenPath); }
		else { IFileManager::Get().Delete(*TokenPath); }
	}
	FAutomationTestBase* Test;
	UGameInstance* Instance = nullptr;
	UWorld* World = nullptr;
	UBlueprint* Blueprint = nullptr;
	UJWNU_SseTestReceiver* Receiver = nullptr;
	UJWNU_SseRequestBase* Request = nullptr;
	TStrongObjectPtr<UJWNU_SseRequestBase> FinishedRequest;
	TStrongObjectPtr<UJWNU_HttpRequest> BootstrapRequest;
	FString BaseURL, TokenPath;
	TArray<uint8> TokenBackup;
	bool bShutdown = false;
	bool bImmediate = false;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_SseImmediateTest, "JWNetworkUtility.SSE.Immediate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_SseImmediateTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UJWNU_SseTestReceiver> Receiver(NewObject<UJWNU_SseTestReceiver>());
    FJWNU_OnSseResponseBP Error;
    Error.BindDynamic(Receiver.Get(), &UJWNU_SseTestReceiver::ReceiveError);
    TestNull(TEXT("Invalid world returns null"), UJWNU_BFL_SseClient::CallSseApi(nullptr, EJWNU_HttpMethod::Get,
        TEXT(""), {}, {}, {}, {}, {}, Error, {}, EJWNU_ServiceType::GameServer, TEXT("/sse/events"), false));
    TestEqual(TEXT("Invalid world callback once"), Receiver->TerminalCount, 1);
    TestEqual(TEXT("Invalid world diagnostic"), Receiver->LastResponse.Error, EJWNU_SseError::InvalidRequest);
    auto* Function = UJWNU_BFL_SseClient::StaticClass()->FindFunctionByName(TEXT("CallSseApi"));
    TestTrue(TEXT("Immediate SSE BP node visible"), UEdGraphSchema_K2::CanUserKismetCallFunction(Function));
    TestTrue(TEXT("Immediate SSE Options default"), Function->GetMetaData(TEXT("AutoCreateRefTerm")).Contains(TEXT("Options")));
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUSseIntegration")))
    { AddInfo(TEXT("Integration skipped; start the FastAPI fixture and pass -JWNUSseIntegration.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::SseTest::FIntegrationCommand(this, true));
    return true;
}
#endif
