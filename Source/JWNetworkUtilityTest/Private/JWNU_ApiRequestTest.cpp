// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_ApiTestReceiver.h"
#include "JWNU_HttpTestReceiver.h"
#include "JWNU_HttpRequest.h"
#include "JWNU_HttpRequestJobHandle.h"
#include "JWNU_SseRequest.h"
#include "JWNU_BFL_ApiClientService.h"
#include "JWNU_GIS_ApiHostProvider.h"
#include "JWNU_GIS_ApiIdentityProvider.h"
#include "JWNU_OpenAILiveSession.h"
#include "JWNU_OpenAILiveComponent.h"
#include "JWNU_TypeSafeRequest.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_ApiNamesTest, "JWNetworkUtility.API.Nodes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_ApiNamesTest::RunTest(const FString& Parameters)
{
    const TArray<TPair<UClass*, FName>> Public = {
        {UJWNU_BFL_ApiClientService::StaticClass(), TEXT("SendHttpRequest")},
        {UJWNU_BFL_ApiClientService::StaticClass(), TEXT("CallApi")},
        {UJWNU_SseRequest::StaticClass(), TEXT("CreateSseRequest")}, {UJWNU_SseApiRequest::StaticClass(), TEXT("CreateSseApiRequest")},
        {UJWNU_HttpRequest::StaticClass(), TEXT("CreateHttpRequest")},
        {UJWNU_ApiRequest::StaticClass(), TEXT("CreateApiRequest")}, {UJWNU_TypeSafeRequest::StaticClass(), TEXT("CreateTypeSafeRequest")},
        {UJWNU_OpenAILiveSession::StaticClass(), TEXT("CreateOpenAILiveSession")}};
    for (const auto& Entry : Public)
    {
        TestTrue(TEXT("Named factory visible"), UEdGraphSchema_K2::CanUserKismetCallFunction(Entry.Key->FindFunctionByName(Entry.Value)));
    }
    for (auto* Class : {UJWNU_SseRequest::StaticClass(), UJWNU_SseApiRequest::StaticClass(), UJWNU_HttpRequest::StaticClass(), UJWNU_ApiRequest::StaticClass(), UJWNU_TypeSafeRequest::StaticClass(), UJWNU_OpenAILiveSession::StaticClass(), UJWNU_OpenAILiveComponent::StaticClass()})
    {
        for (const TCHAR* Name : {TEXT("Start"), TEXT("Cancel")})
        { TestTrue(TEXT("Common lifecycle visible"), UEdGraphSchema_K2::CanUserKismetCallFunction(Class->FindFunctionByName(Name))); }
        if (Class != UJWNU_OpenAILiveComponent::StaticClass())
        { TestTrue(TEXT("Request/session active query visible"), UEdGraphSchema_K2::CanUserKismetCallFunction(Class->FindFunctionByName(TEXT("IsActive")))); }
    }
    const TArray<TPair<UClass*, FName>> Legacy = {
        {UJWNU_HttpRequestJobHandle::StaticClass(), TEXT("Cancel")}, {UJWNU_HttpRequestJobHandle::StaticClass(), TEXT("IsActive")},
        {UJWNU_TypeSafeRequest::StaticClass(), TEXT("CreateRequest")},
        {UJWNU_OpenAILiveSession::StaticClass(), TEXT("CreateLiveSession")}, {UJWNU_OpenAILiveSession::StaticClass(), TEXT("Abort")},
        {UJWNU_HttpRequestJobHandle::StaticClass(), TEXT("IsRunning")}, {UJWNU_OpenAILiveComponent::StaticClass(), TEXT("StartLive")},
        {UJWNU_OpenAILiveComponent::StaticClass(), TEXT("StartLiveFromEnvironment")}, {UJWNU_OpenAILiveComponent::StaticClass(), TEXT("StopLive")}};
    for (const auto& Entry : Legacy)
    {
        auto* Function = Entry.Key->FindFunctionByName(Entry.Value);
        TestNull(TEXT("Removed legacy node no longer resolves"), Function);
    }
    TestFalse(TEXT("Transport handle is not a BP variable type"), UJWNU_HttpRequestJobHandle::StaticClass()->GetBoolMetaData(TEXT("BlueprintType")));
    TestFalse(TEXT("Transport job is not a BP variable type"), UJWNU_HttpRequestJob::StaticClass()->GetBoolMetaData(TEXT("BlueprintType")));
    TStrongObjectPtr<UJWNU_ApiTestReceiver> InvalidWorldReceiver(NewObject<UJWNU_ApiTestReceiver>());
    FOnHttpResponseBPEvent Response;
    Response.BindDynamic(InvalidWorldReceiver.Get(), &UJWNU_ApiTestReceiver::ImmediateCallback);
    TestNull(TEXT("Immediate call rejects invalid world"), UJWNU_BFL_ApiClientService::CallApi(nullptr, EJWNU_ServiceType::GameServer,
        EJWNU_HttpMethod::Get, TEXT("/test/api-request"), TEXT(""), {}, Response, {}, false));
    TestEqual(TEXT("Invalid world callback delivered once"), InvalidWorldReceiver->FailedCount, 1);
    TestTrue(TEXT("Invalid world has diagnostic body"), InvalidWorldReceiver->Error.Response.ResponseBody.Contains(TEXT("START_FAILED")));
    TStrongObjectPtr<UJWNU_HttpTestReceiver> InvalidHttpReceiver(NewObject<UJWNU_HttpTestReceiver>());
    Response.BindDynamic(InvalidHttpReceiver.Get(), &UJWNU_HttpTestReceiver::ImmediateCallback);
    TestNull(TEXT("Immediate HTTP rejects invalid world"), UJWNU_BFL_ApiClientService::SendHttpRequest(nullptr,
        EJWNU_HttpMethod::Get, TEXT("http://127.0.0.1:18573/health"), TEXT(""), TEXT(""), {}, Response, {}));
    TestEqual(TEXT("Invalid HTTP world callback delivered once"), InvalidHttpReceiver->FailedCount, 1);
    TestTrue(TEXT("Invalid HTTP world diagnostic"), InvalidHttpReceiver->ImmediateBody.Contains(TEXT("START_FAILED")));
    return true;
}

namespace JWNU::ApiTest
{
inline UBlueprint* BuildReceiver(FAutomationTestBase* Test)
{
    auto* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_ApiTestReceiver::StaticClass(), GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_ApiAutomation")), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("ReceiveGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
    auto* Event = NewObject<UK2Node_Event>(Graph);
    Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UJWNU_ApiTestReceiver, Completed), UJWNU_ApiTestReceiver::StaticClass());
    Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
    auto* Record = NewObject<UK2Node_CallFunction>(Graph);
    Record->SetFromFunction(UJWNU_ApiTestReceiver::StaticClass()->FindFunctionByName(TEXT("Record")));
    Graph->AddNode(Record); Record->CreateNewGuid(); Record->AllocateDefaultPins();
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    auto* ImmediateEvent = NewObject<UK2Node_Event>(Graph);
    ImmediateEvent->EventReference.SetExternalMember(TEXT("ImmediateResponse"), UJWNU_ApiTestReceiver::StaticClass());
    ImmediateEvent->bOverrideFunction = true; Graph->AddNode(ImmediateEvent); ImmediateEvent->CreateNewGuid(); ImmediateEvent->AllocateDefaultPins();
    auto* ImmediateRecord = NewObject<UK2Node_CallFunction>(Graph);
    ImmediateRecord->SetFromFunction(UJWNU_ApiTestReceiver::StaticClass()->FindFunctionByName(TEXT("RecordImmediate")));
    Graph->AddNode(ImmediateRecord); ImmediateRecord->CreateNewGuid(); ImmediateRecord->AllocateDefaultPins();
    Test->TestTrue(TEXT("Immediate BP exec wire"), Schema->TryCreateConnection(ImmediateEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), ImmediateRecord->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
    for (const TCHAR* Name : {TEXT("StatusCode"), TEXT("ResponseBody")})
    { Test->TestTrue(TEXT("Immediate BP response wire"), Schema->TryCreateConnection(ImmediateEvent->FindPinChecked(Name), ImmediateRecord->FindPinChecked(Name))); }
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
            auto* Hosts = Instance->GetSubsystem<UJWNU_GIS_ApiHostProvider>();
            HostMap = FindFProperty<FMapProperty>(Hosts->GetClass(), TEXT("ServiceTypeToHostMap"))->ContainerPtrToValuePtr<TMap<EJWNU_ServiceType, FString>>(Hosts);
            HostMap->Add(EJWNU_ServiceType::GameServer, Base);
            BP = BuildReceiver(Test); BP->AddToRoot();
        }
        if (Stage == 9) { Cleanup(); return true; }
        if (!Receiver) { Start(); }
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 8) { Test->AddError(TEXT("API stage timed out")); Cleanup(); return true; }
        if (!bAction && Now - Started > .15)
        {
            bAction = true;
            if (Stage == 3) { CollectGarbage(RF_NoFlags); Test->TestTrue(TEXT("Active request survived GC"), IsValid(Request) && Request->IsActive()); }
            if (Stage == 4) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); }
            if (Stage == 8) { Instance->Shutdown(); bShutdown = true; }
        }
        if (Receiver->CompletedCount + Receiver->FailedCount == 0) { return false; }
        if (Finished == 0) { Finished = Now; }
        if (Now - Finished < 1.1) { return false; }
        const bool bSuccess = Stage == 0 || Stage == 3 || Stage == 7;
        Test->TestEqual(TEXT("One BP terminal event"), Receiver->CompletedCount + Receiver->FailedCount, 1);
        if (!bImmediate) { Test->TestEqual(TEXT("One native terminal event"), NativeCount, 1); }
        Test->TestEqual(TEXT("Expected success"), Receiver->CompletedCount, bSuccess ? 1 : 0);
        Test->TestFalse(TEXT("Terminal inactive"), Request->IsActive());
        Test->TestFalse(TEXT("Single use request"), Request->Start(EJWNU_ServiceType::GameServer, EJWNU_HttpMethod::Get, TEXT("/health"), TEXT(""), {}, false));
        if (bSuccess) { Test->TestTrue(TEXT("Unicode query through API and BP"), Receiver->Result.ResponseBody.Contains(TEXT("편대 복귀"))); }
        else
        {
            const bool bCancelled = Stage == 2 || Stage == 4 || Stage == 8;
            Test->TestEqual(TEXT("Failure reason"), Receiver->Error.Code, bCancelled ? EJWNU_ApiRequestError::Cancelled : EJWNU_ApiRequestError::RequestFailed);
        }
        Receiver->RemoveFromRoot(); Receiver = nullptr; Request = nullptr; FinishedRequest.Reset(); ++Stage;
        return false;
    }
private:
    void Start()
    {
        Test->AddInfo(FString::Printf(TEXT("API stage %d"), Stage));
        Started = FPlatformTime::Seconds(); Finished = 0; bAction = false; NativeCount = 0;
        Receiver = NewObject<UJWNU_ApiTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get()); Receiver->AddToRoot();
        if (!bImmediate)
        {
            Request = UJWNU_ApiRequest::CreateApiRequest(World);
            Test->TestNotNull(TEXT("Factory creates request"), Request);
            Test->TestFalse(TEXT("Create does not start"), Request->IsActive());
            Request->OnCompleted.AddDynamic(Receiver, &UJWNU_ApiTestReceiver::Completed);
            Request->OnFailed.AddDynamic(Receiver, &UJWNU_ApiTestReceiver::Failed);
            BindNative();
        }
        if (Stage == 5) { HostMap->Remove(EJWNU_ServiceType::GameServer); }
        if (Stage == 6)
        {
            auto* Identity = Instance->GetSubsystem<UJWNU_GIS_ApiIdentityProvider>();
            auto* Tokens = FindFProperty<FMapProperty>(Identity->GetClass(), TEXT("ServiceTypeToTokenContainerMap"))->ContainerPtrToValuePtr<TMap<EJWNU_ServiceType, FJWNU_AccessTokenContainer>>(Identity);
            Tokens->Remove(EJWNU_ServiceType::GameServer);
        }
        const TMap<FString, FString> Query = {{TEXT("text"), TEXT("편대 복귀")}, {TEXT("delay"), Stage == 2 || Stage == 3 || Stage == 4 || Stage == 8 ? TEXT(".8") : TEXT("0")}, {TEXT("status"), Stage == 1 ? TEXT("404") : (Stage == 7 ? TEXT("206") : TEXT("200"))}};
        if (bImmediate)
        {
            FOnHttpResponseBPEvent Response;
            Response.BindDynamic(Receiver, &UJWNU_ApiTestReceiver::ImmediateCallback);
            Request = UJWNU_BFL_ApiClientService::CallApi(World, EJWNU_ServiceType::GameServer, EJWNU_HttpMethod::Get, TEXT("/test/api-request"), TEXT(""), Query, Response, {}, Stage == 6);
            Test->TestNotNull(TEXT("Immediate call returns control request"), Request);
            if (!Request->IsActive()) { FinishedRequest.Reset(Request); }
            BindNative();
        }
        else
        {
            const bool bAccepted = Request->Start(EJWNU_ServiceType::GameServer, EJWNU_HttpMethod::Get, TEXT("/test/api-request"), TEXT(""), Query, Stage == 6);
            Test->TestEqual(TEXT("Start acceptance"), bAccepted, Stage != 5 && Stage != 6);
        }
        if (Stage == 5) { HostMap->Add(EJWNU_ServiceType::GameServer, Base); }
        if (Stage == 2) { Request->Cancel(); Request->Cancel(); CollectGarbage(RF_NoFlags); }
    }
    void BindNative()
    {
        Request->OnCompletedNative.AddLambda([this](const FJWNU_ApiResult&) { FinishedRequest.Reset(Request); ++NativeCount; Request->Cancel(); CollectGarbage(RF_NoFlags); });
        Request->OnFailedNative.AddLambda([this](const FJWNU_ApiError&) { FinishedRequest.Reset(Request); ++NativeCount; });
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
    UJWNU_ApiRequest* Request = nullptr;
    TStrongObjectPtr<UJWNU_ApiRequest> FinishedRequest;
    UJWNU_ApiTestReceiver* Receiver = nullptr;
    TMap<EJWNU_ServiceType, FString>* HostMap = nullptr;
    FString Base;
    int32 Stage = 0, NativeCount = 0;
    double Started = 0, Finished = 0;
    bool bAction = false, bShutdown = false;
    bool bImmediate = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_ApiIntegrationTest, "JWNetworkUtility.API.FastAPI", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_ApiIntegrationTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUApiIntegration"))) { AddInfo(TEXT("Pass -JWNUApiIntegration with a local fixture server.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::ApiTest::FIntegration(this)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_ApiImmediateTest, "JWNetworkUtility.API.Immediate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_ApiImmediateTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUApiIntegration"))) { AddInfo(TEXT("Pass -JWNUApiIntegration with a local fixture server.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::ApiTest::FIntegration(this, true)); return true;
}
#endif
