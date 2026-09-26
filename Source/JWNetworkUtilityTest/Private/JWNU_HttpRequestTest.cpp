// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_HttpTestReceiver.h"
#include "JWNU_BFL_ApiClientService.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"

namespace JWNU::HttpTest
{
inline UBlueprint* BuildReceiver(FAutomationTestBase* Test)
{
    auto* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_HttpTestReceiver::StaticClass(), GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_HttpAutomation")), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("ReceiveGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
    auto* Event = NewObject<UK2Node_Event>(Graph);
    Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UJWNU_HttpTestReceiver, Completed), UJWNU_HttpTestReceiver::StaticClass());
    Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
    auto* Record = NewObject<UK2Node_CallFunction>(Graph);
    Record->SetFromFunction(UJWNU_HttpTestReceiver::StaticClass()->FindFunctionByName(TEXT("Record")));
    Graph->AddNode(Record); Record->CreateNewGuid(); Record->AllocateDefaultPins();
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    auto* ImmediateEvent = NewObject<UK2Node_Event>(Graph);
    ImmediateEvent->EventReference.SetExternalMember(TEXT("ImmediateResponse"), UJWNU_HttpTestReceiver::StaticClass());
    ImmediateEvent->bOverrideFunction = true; Graph->AddNode(ImmediateEvent); ImmediateEvent->CreateNewGuid(); ImmediateEvent->AllocateDefaultPins();
    auto* ImmediateRecord = NewObject<UK2Node_CallFunction>(Graph);
    ImmediateRecord->SetFromFunction(UJWNU_HttpTestReceiver::StaticClass()->FindFunctionByName(TEXT("RecordImmediate")));
    Graph->AddNode(ImmediateRecord); ImmediateRecord->CreateNewGuid(); ImmediateRecord->AllocateDefaultPins();
    Test->TestTrue(TEXT("Immediate HTTP BP exec"), Schema->TryCreateConnection(ImmediateEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), ImmediateRecord->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
    for (const TCHAR* Name : {TEXT("StatusCode"), TEXT("ResponseBody")})
    { Test->TestTrue(TEXT("Immediate HTTP BP data"), Schema->TryCreateConnection(ImmediateEvent->FindPinChecked(Name), ImmediateRecord->FindPinChecked(Name))); }
    auto* StartEvent = NewObject<UK2Node_Event>(Graph);
    StartEvent->EventReference.SetExternalMember(TEXT("StartDefaults"), UJWNU_HttpTestReceiver::StaticClass());
    StartEvent->bOverrideFunction = true; Graph->AddNode(StartEvent); StartEvent->CreateNewGuid(); StartEvent->AllocateDefaultPins();
    auto* StartCall = NewObject<UK2Node_CallFunction>(Graph);
    StartCall->SetFromFunction(UJWNU_HttpRequest::StaticClass()->FindFunctionByName(TEXT("Start")));
    Graph->AddNode(StartCall); StartCall->CreateNewGuid(); StartCall->AllocateDefaultPins();
    Test->TestTrue(TEXT("HTTP BP Start exec"), Schema->TryCreateConnection(StartEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
    Test->TestTrue(TEXT("HTTP BP Start target"), Schema->TryCreateConnection(StartEvent->FindPinChecked(TEXT("Target")), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Self)));
    Test->TestTrue(TEXT("HTTP BP Start URL"), Schema->TryCreateConnection(StartEvent->FindPinChecked(TEXT("URL")), StartCall->FindPinChecked(TEXT("URL"))));
    Test->TestTrue(TEXT("HTTP QueryParams unconnected"), StartCall->FindPinChecked(TEXT("QueryParams"))->LinkedTo.IsEmpty());
    Test->TestTrue(TEXT("API BP result wire"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Value")), Record->FindPinChecked(TEXT("Value"))));
    Test->TestTrue(TEXT("API BP exec wire"), Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Record->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
    FKismetEditorUtilities::CompileBlueprint(BP);
    Test->TestTrue(TEXT("API BP compiles"), BP->Status != BS_Error);
    return BP;
}

class FIntegration : public IAutomationLatentCommand
{
public:
    explicit FIntegration(FAutomationTestBase* InTest, bool bInImmediate = false) : Test(InTest), bImmediate(bInImmediate) {}
    virtual bool Update() override
    {
        if (!Instance)
        {
            Base = TEXT("http://127.0.0.1:18573"); FParse::Value(FCommandLine::Get(), TEXT("JWNUApiTestURL="), Base);
            Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone(); World = Instance->GetWorld();
            BP = BuildReceiver(Test); BP->AddToRoot();
        }
        if (Stage == 10) { Cleanup(); return true; }
        if (!Receiver) { Start(); }
        if (!bShutdown) { World->GetTimerManager().Tick(.02f); }
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 8) { Test->AddError(TEXT("HTTP stage timed out")); Cleanup(); return true; }
        if (!bAction && Now - Started > .15)
        {
            bAction = true;
            if (Stage == 3) { CollectGarbage(RF_NoFlags); Test->TestTrue(TEXT("Active request survived GC"), IsValid(Request) && Request->IsActive()); }
            if (Stage == 4) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); }
            if (Stage == 9) { Instance->Shutdown(); bShutdown = true; }
        }
        if (Receiver->CompletedCount + Receiver->FailedCount == 0) { return false; }
        if (Finished == 0) { Finished = Now; }
        if (Now - Finished < 1.1) { return false; }
        const bool bSuccess = Stage == 0 || Stage == 3 || Stage == 7 || Stage == 8;
        Test->TestEqual(TEXT("One BP terminal event"), Receiver->CompletedCount + Receiver->FailedCount, 1);
        if (!bImmediate) { Test->TestEqual(TEXT("One native terminal event"), NativeCount, 1); }
        Test->TestEqual(TEXT("Expected success"), Receiver->CompletedCount, bSuccess ? 1 : 0);
        Test->TestFalse(TEXT("Terminal inactive"), Request->IsActive());
        Test->TestEqual(TEXT("HTTP common terminal state"), Request->GetState(), bSuccess ? EJWNU_RequestState::Succeeded :
            (Request->GetError().Code == EJWNU_HttpRequestError::Cancelled ? EJWNU_RequestState::Cancelled : EJWNU_RequestState::Failed));
        Test->TestFalse(TEXT("Single use request"), Request->Start(EJWNU_HttpMethod::Get, Base + TEXT("/health"), TEXT(""), {}));
        if (bImmediate)
        {
            Receiver->Result = Request->GetResult(); Receiver->Error = Request->GetError();
            if (bSuccess || Stage == 1 || Stage == 6)
            {
                const auto Raw = bSuccess ? Receiver->Result : Receiver->Error.Response;
                Test->TestEqual(TEXT("Immediate HTTP enum status"), Receiver->ImmediateStatus, JWNU_IntToHttpStatusCode(Raw.StatusCode));
                Test->TestEqual(TEXT("Immediate HTTP preserves raw body"), Receiver->ImmediateBody, Raw.ResponseBody);
            }
            else
            {
                Test->TestEqual(TEXT("Immediate HTTP local status"), Receiver->ImmediateStatus, EJWNU_HttpStatusCode::None);
                Test->TestTrue(TEXT("Immediate HTTP local diagnostic"), Receiver->ImmediateBody.Contains(Stage == 5 ? TEXT("START_FAILED") : TEXT("CANCELLED")));
            }
        }
        if (bSuccess)
        {
            Test->TestTrue(TEXT("Raw response through BP"), Receiver->Result.ResponseBody.Contains(TEXT("안녕 API")));
            if (Stage == 3 || Stage == 7) { Test->TestTrue(TEXT("Unicode query preserved"), Receiver->Result.ResponseBody.Contains(TEXT("편대 복귀"))); }
            if (Stage == 7) { Test->TestEqual(TEXT("Exact status preserved"), Receiver->Result.StatusCode, 206); }
            if (Stage == 8)
            {
                Test->TestTrue(TEXT("POST body preserved"), Receiver->Result.ResponseBody.Contains(TEXT("request-body")));
                Test->TestTrue(TEXT("Bearer key received"), Receiver->Result.ResponseBody.Contains(TEXT("fixture-key-ok")));
            }
        }
        else
        {
            const bool bCancelled = Stage == 2 || Stage == 4 || Stage == 9;
            Test->TestEqual(TEXT("Failure reason"), Receiver->Error.Code, bCancelled ? EJWNU_HttpRequestError::Cancelled : EJWNU_HttpRequestError::RequestFailed);
            if (Stage == 1 || Stage == 6)
            {
                Test->TestEqual(TEXT("Raw error status"), Receiver->Error.Response.StatusCode, Stage == 1 ? 404 : 503);
                Test->TestTrue(TEXT("Raw error body not normalized"), Receiver->Error.Response.ResponseBody.Contains(TEXT("안녕 API")));
            }
            if (Stage == 6) { Test->TestTrue(TEXT("Retry event delivered"), Receiver->RetryCount > 0); }
        }
        Receiver->RemoveFromRoot(); Receiver = nullptr; Request = nullptr; FinishedRequest.Reset(); ++Stage;
        return false;
    }
private:
    void Start()
    {
        Test->AddInfo(FString::Printf(TEXT("HTTP stage %d"), Stage));
        Started = FPlatformTime::Seconds(); Finished = 0; bAction = false; NativeCount = 0;
        Receiver = NewObject<UJWNU_HttpTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get()); Receiver->AddToRoot();
        if (!bImmediate)
        {
            Request = UJWNU_HttpRequest::CreateHttpRequest(World);
            Test->TestNotNull(TEXT("Factory creates request"), Request);
            Test->TestFalse(TEXT("Create does not start"), Request->IsActive());
            Request->OnCompleted.AddDynamic(Receiver, &UJWNU_HttpTestReceiver::Completed);
            Request->OnFailed.AddDynamic(Receiver, &UJWNU_HttpTestReceiver::Failed);
            Request->OnRetry.AddDynamic(Receiver, &UJWNU_HttpTestReceiver::Retry);
            BindNative();
            if (Stage == 0) { Receiver->StartDefaults(Request, Base + TEXT("/test/api-request")); return; }
        }
        const TMap<FString, FString> Query = {{TEXT("text"), TEXT("편대 복귀")}, {TEXT("delay"), Stage == 2 || Stage == 3 || Stage == 4 || Stage == 9 ? TEXT(".8") : TEXT("0")}, {TEXT("status"), Stage == 1 ? TEXT("404") : (Stage == 6 ? TEXT("503") : (Stage == 7 ? TEXT("206") : TEXT("200")))}};
        if (bImmediate)
        {
            FOnHttpResponseBPEvent Response; Response.BindDynamic(Receiver, &UJWNU_HttpTestReceiver::ImmediateCallback);
            FOnHttpRequestJobRetryBPEvent Retry; Retry.BindDynamic(Receiver, &UJWNU_HttpTestReceiver::Retry);
            Request = UJWNU_BFL_ApiClientService::SendHttpRequest(World, Stage == 8 ? EJWNU_HttpMethod::Post : EJWNU_HttpMethod::Get,
                Stage == 5 ? TEXT("/relative-url") : Base + TEXT("/test/api-request"), Stage == 8 ? TEXT("fixture-key") : TEXT(""),
                Stage == 8 ? TEXT("{\"value\":\"request-body\"}") : TEXT(""), Query, Response, Retry);
            Test->TestNotNull(TEXT("Immediate HTTP returns control request"), Request);
            if (!Request->IsActive()) { FinishedRequest.Reset(Request); }
            BindNative();
        }
        else
        {
            const bool bAccepted = Request->Start(Stage == 8 ? EJWNU_HttpMethod::Post : EJWNU_HttpMethod::Get,
                Stage == 5 ? TEXT("/relative-url") : Base + TEXT("/test/api-request"),
                Stage == 8 ? TEXT("{\"value\":\"request-body\"}") : TEXT(""), Query, Stage == 8 ? TEXT("fixture-key") : TEXT(""));
            Test->TestEqual(TEXT("Start acceptance"), bAccepted, Stage != 5);
        }
        if (Stage == 2) { Request->Cancel(); Request->Cancel(); CollectGarbage(RF_NoFlags); }
    }
    void BindNative()
    {
        Request->OnCompletedNative.AddLambda([this](const FJWNU_HttpResult&) { FinishedRequest.Reset(Request); ++NativeCount; Request->Cancel(); CollectGarbage(RF_NoFlags); });
        Request->OnFailedNative.AddLambda([this](const FJWNU_HttpError&) { FinishedRequest.Reset(Request); ++NativeCount; });
    }
    void Cleanup()
    {
        if (Request) { Request->Cancel(); }
        if (!bShutdown) { Instance->Shutdown(); }
        if (Receiver) { Receiver->RemoveFromRoot(); }
        GEngine->DestroyWorldContext(World); World->DestroyWorld(false); Instance->RemoveFromRoot(); BP->RemoveFromRoot();
    }
    FAutomationTestBase* Test;
    UGameInstance* Instance = nullptr;
    UWorld* World = nullptr;
    UBlueprint* BP = nullptr;
    UJWNU_HttpRequest* Request = nullptr;
    TStrongObjectPtr<UJWNU_HttpRequest> FinishedRequest;
    UJWNU_HttpTestReceiver* Receiver = nullptr;
    FString Base;
    int32 Stage = 0, NativeCount = 0;
    double Started = 0, Finished = 0;
    bool bAction = false, bShutdown = false;
    bool bImmediate = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_HttpIntegrationTest, "JWNetworkUtility.API.HTTP", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_HttpIntegrationTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUApiIntegration"))) { AddInfo(TEXT("Pass -JWNUApiIntegration with a local fixture server.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::HttpTest::FIntegration(this)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_HttpImmediateTest, "JWNetworkUtility.API.HTTPImmediate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_HttpImmediateTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUApiIntegration"))) { AddInfo(TEXT("Pass -JWNUApiIntegration with a local fixture server.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::HttpTest::FIntegration(this, true)); return true;
}
#endif
