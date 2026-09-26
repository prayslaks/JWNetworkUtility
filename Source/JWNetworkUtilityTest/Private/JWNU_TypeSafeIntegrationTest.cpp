// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_TypeSafeTestReceiver.h"
#include "JWNU_TypeSafeRequest.h"
#include "JWNU_BFL_TypeSafe.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeMap.h"
#include "Kismet/KismetSystemLibrary.h"

namespace JWNU::TypeSafeTest
{
inline UBlueprint* BuildReceiver(FAutomationTestBase* Test)
{
    for (const TCHAR* Name : {TEXT("Evaluate"), TEXT("EvaluateFromEnvironment")})
    {
        const auto* Function = UJWNU_BFL_TypeSafe::StaticClass()->FindFunctionByName(Name);
        Test->TestNull(TEXT("Removed evaluation no longer resolves"), Function);
    }
    for (const TCHAR* Name : {TEXT("CreateTypeSafeRequest"), TEXT("Start"), TEXT("StartFromEnvironment")})
    {
        Test->TestTrue(TEXT("Explicit request lifecycle is available in BP"), UEdGraphSchema_K2::CanUserKismetCallFunction(UJWNU_TypeSafeRequest::StaticClass()->FindFunctionByName(Name)));
    }
    auto* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_TypeSafeTestReceiver::StaticClass(), GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_TypeSafeAutomation")),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("ReceiveGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
    auto* Event = NewObject<UK2Node_Event>(Graph);
    Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UJWNU_TypeSafeTestReceiver, Completed), UJWNU_TypeSafeTestReceiver::StaticClass());
    Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
    auto* Record = NewObject<UK2Node_CallFunction>(Graph);
    Record->SetFromFunction(UJWNU_TypeSafeTestReceiver::StaticClass()->FindFunctionByName(TEXT("Record")));
    Graph->AddNode(Record); Record->CreateNewGuid(); Record->AllocateDefaultPins();
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    // 저장된 Credential 핀을 재현해 새 API Key 핀으로 연결과 직접 입력값이 이동하는지 확인한다.
    auto AddCall = [Graph](UFunction* Function)
    {
        auto* Node = NewObject<UK2Node_CallFunction>(Graph);
        Node->SetFromFunction(Function); Graph->AddNode(Node); Node->CreateNewGuid(); Node->AllocateDefaultPins(); return Node;
    };
    auto* KeyLiteral = AddCall(UKismetSystemLibrary::StaticClass()->FindFunctionByName(TEXT("MakeLiteralString")));
    for (UFunction* Function : {UJWNU_TypeSafeRequest::StaticClass()->FindFunctionByName(TEXT("Start"))})
    {
        auto* Wired = AddCall(Function);
        Test->TestTrue(TEXT("API key wire created"), Schema->TryCreateConnection(KeyLiteral->GetReturnValuePin(), Wired->FindPinChecked(TEXT("ApiKey"))));
        Wired->FindPinChecked(TEXT("ApiKey"))->PinName = TEXT("Credential");
        Wired->ReconstructNode();
        Test->TestTrue(TEXT("Old Credential wire preserved"), Wired->FindPinChecked(TEXT("ApiKey"))->LinkedTo.Contains(KeyLiteral->GetReturnValuePin()));
        auto* Literal = AddCall(Function);
        Schema->TrySetDefaultValue(*Literal->FindPinChecked(TEXT("ApiKey")), TEXT("fixture-not-a-real-key"));
        Literal->FindPinChecked(TEXT("ApiKey"))->PinName = TEXT("Credential");
        Literal->ReconstructNode();
        Test->TestEqual(TEXT("Old Credential literal preserved"), Literal->FindPinChecked(TEXT("ApiKey"))->DefaultValue, FString(TEXT("fixture-not-a-real-key")));
    }
    // 실제 Start 호출 노드를 연결해 Options 미연결 상태에서 BP 컴파일과 기본값 생성을 검증한다.
    for (bool bEnvironment : {false, true})
    {
        auto* StartEvent = NewObject<UK2Node_Event>(Graph);
        StartEvent->EventReference.SetExternalMember(bEnvironment ? TEXT("StartEnvironmentWithDefaultOptions") : TEXT("StartWithDefaultOptions"), UJWNU_TypeSafeTestReceiver::StaticClass());
        StartEvent->bOverrideFunction = true; Graph->AddNode(StartEvent); StartEvent->CreateNewGuid(); StartEvent->AllocateDefaultPins();
        auto* StartCall = NewObject<UK2Node_CallFunction>(Graph);
        StartCall->SetFromFunction(UJWNU_TypeSafeRequest::StaticClass()->FindFunctionByName(bEnvironment ? TEXT("StartFromEnvironment") : TEXT("Start")));
        Graph->AddNode(StartCall); StartCall->CreateNewGuid(); StartCall->AllocateDefaultPins();
        Test->TestTrue(TEXT("Default Options call exec"), Schema->TryCreateConnection(StartEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        Test->TestTrue(TEXT("Default Options call target"), Schema->TryCreateConnection(StartEvent->FindPinChecked(TEXT("Request")), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Self)));
        for (const TCHAR* Name : {TEXT("State"), TEXT("Questions")})
        { Test->TestTrue(TEXT("Required input connected"), Schema->TryCreateConnection(StartEvent->FindPinChecked(Name), StartCall->FindPinChecked(Name))); }
        Test->TestTrue(TEXT("Options intentionally unconnected"), StartCall->FindPinChecked(TEXT("Options"))->LinkedTo.IsEmpty());
        Test->TestFalse(TEXT("Options uses a default value"), StartCall->FindPinChecked(TEXT("Options"))->bDefaultValueIsIgnored);
    }
    for (bool bEnvironment : {false, true})
    {
        auto* CallEvent = NewObject<UK2Node_Event>(Graph);
        CallEvent->EventReference.SetExternalMember(bEnvironment ? TEXT("CallEnvironmentWithDefaultOptions") : TEXT("CallWithDefaultOptions"), UJWNU_TypeSafeTestReceiver::StaticClass());
        CallEvent->bOverrideFunction = true; Graph->AddNode(CallEvent); CallEvent->CreateNewGuid(); CallEvent->AllocateDefaultPins();
        auto* Call = AddCall(UJWNU_BFL_TypeSafe::StaticClass()->FindFunctionByName(bEnvironment ? TEXT("CallTypeSafeApiFromEnvironment") : TEXT("CallTypeSafeApi")));
        auto* Save = AddCall(UJWNU_TypeSafeTestReceiver::StaticClass()->FindFunctionByName(TEXT("RecordStartedRequest")));
        Test->TestTrue(TEXT("Immediate BP exec"), Schema->TryCreateConnection(CallEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), Call->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        Test->TestTrue(TEXT("Immediate world context"), Schema->TryCreateConnection(CallEvent->FindPinChecked(TEXT("Context")), Call->FindPinChecked(TEXT("WorldContextObject"))));
        for (const TCHAR* Name : {TEXT("State"), TEXT("Questions")})
        { Test->TestTrue(TEXT("Immediate inputs"), Schema->TryCreateConnection(CallEvent->FindPinChecked(Name), Call->FindPinChecked(Name))); }
        for (const TCHAR* Name : {TEXT("Options"), TEXT("OnCompleted"), TEXT("OnFailed")})
        { Test->TestTrue(TEXT("Immediate optional pin unconnected"), Call->FindPinChecked(Name)->LinkedTo.IsEmpty()); }
        Test->TestTrue(TEXT("Immediate save exec"), Schema->TryCreateConnection(Call->FindPinChecked(UEdGraphSchema_K2::PN_Then), Save->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        Test->TestTrue(TEXT("Immediate return saved"), Schema->TryCreateConnection(Call->GetReturnValuePin(), Save->FindPinChecked(TEXT("Request"))));
    }
    Test->TestTrue(TEXT("BP typed result wire"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Result")), Record->FindPinChecked(TEXT("Result"))));
    Test->TestTrue(TEXT("BP exec wire"), Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Record->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
    auto MakeCall = [Graph](const TCHAR* Name)
    {
        auto* Node = NewObject<UK2Node_CallFunction>(Graph);
        Node->SetFromFunction(UJWNU_BFL_TypeSafe::StaticClass()->FindFunctionByName(Name));
        Graph->AddNode(Node); Node->CreateNewGuid(); Node->AllocateDefaultPins(); return Node;
    };
    auto* WithCriteria = MakeCall(TEXT("MakeNoulQuestion"));
    auto* WithoutCriteria = MakeCall(TEXT("MakeNoulQuestion"));
    auto* Criteria = NewObject<UK2Node_MakeStruct>(Graph);
    Criteria->StructType = FJWNU_TypeSafeNoulCriteria::StaticStruct();
    Graph->AddNode(Criteria); Criteria->CreateNewGuid(); Criteria->AllocateDefaultPins();
    Schema->TrySetDefaultValue(*Criteria->FindPinChecked(TEXT("TrueDescription")), TEXT("이전 문의 있음"));
    Schema->TrySetDefaultValue(*Criteria->FindPinChecked(TEXT("FalseDescription")), TEXT("이전 문의 없음"));
    UEdGraphPin* CriteriaOutput = nullptr;
    for (auto* Pin : Criteria->Pins) { if (Pin->Direction == EGPD_Output) { CriteriaOutput = Pin; break; } }
    Test->TestTrue(TEXT("Noul criteria struct wire"), Schema->TryCreateConnection(CriteriaOutput, WithCriteria->FindPinChecked(TEXT("Criteria"))));
    Test->TestTrue(TEXT("Noul with criteria consumed"), Schema->TryCreateConnection(WithCriteria->GetReturnValuePin(), Record->FindPinChecked(TEXT("WithCriteria"))));
    Test->TestTrue(TEXT("Noul default criteria consumed"), Schema->TryCreateConnection(WithoutCriteria->GetReturnValuePin(), Record->FindPinChecked(TEXT("WithoutCriteria"))));
    // 저장된 옛 핀 이름을 재현해 연결이 Criteria로 이동하는지 검증한다.
    auto* Score = MakeCall(TEXT("MakeScoreQuestion"));
    auto* Levels = NewObject<UK2Node_MakeArray>(Graph);
    Graph->AddNode(Levels); Levels->CreateNewGuid(); Levels->AllocateDefaultPins();
    Test->TestTrue(TEXT("Score array connected"), Schema->TryCreateConnection(Levels->GetOutputPin(), Score->FindPinChecked(TEXT("Criteria"))));
    Score->FindPinChecked(TEXT("Criteria"))->PinName = TEXT("Levels");
    Score->ReconstructNode();
    Test->TestTrue(TEXT("Old Levels connection preserved"), Score->FindPinChecked(TEXT("Criteria"))->LinkedTo.Contains(Levels->GetOutputPin()));
    auto* Choice = MakeCall(TEXT("MakeChoiceQuestion"));
    auto* Options = NewObject<UK2Node_MakeMap>(Graph);
    Graph->AddNode(Options); Options->CreateNewGuid(); Options->AllocateDefaultPins();
    Test->TestTrue(TEXT("Choice map connected"), Schema->TryCreateConnection(Options->GetOutputPin(), Choice->FindPinChecked(TEXT("Criteria"))));
    Choice->FindPinChecked(TEXT("Criteria"))->PinName = TEXT("Options");
    Choice->ReconstructNode();
    Test->TestTrue(TEXT("Old Options connection preserved"), Choice->FindPinChecked(TEXT("Criteria"))->LinkedTo.Contains(Options->GetOutputPin()));
    FKismetEditorUtilities::CompileBlueprint(BP);
    Test->TestTrue(TEXT("TypeSafe receiver BP compiled"), BP->Status != BS_Error);
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
            Base = TEXT("http://127.0.0.1:18573");
            FParse::Value(FCommandLine::Get(), TEXT("JWNUTypeSafeTestURL="), Base);
            Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone();
            World = Instance->GetWorld(); BP = BuildReceiver(Test); BP->AddToRoot();
        }
        if (Stage == 25) { Cleanup(); return true; }
        if (!Receiver) { Start(); }
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 8)
        { Test->AddError(FString::Printf(TEXT("TypeSafe stage %d timed out"), Stage)); Cleanup(); return true; }
        if (!bAction && Now - Started > .15)
        {
            bAction = true;
            if (Stage == 11) { Request->Cancel(); Request->Cancel(); }
            if (Stage == 12) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); }
            if (Stage == 20)
            {
                CollectGarbage(RF_NoFlags);
                Test->TestTrue(TEXT("Active request and HTTP job retained through GC"), IsValid(Request) && Request->IsActive());
            }
            if (Stage == 24) { Instance->Shutdown(); bShutdown = true; }
        }
        if (Receiver->CompletedCount + Receiver->FailedCount == 0) { return false; }
        if (!Finished) { Finished = Now; return false; }
        const double Wait = Stage == 11 || Stage == 12 || Stage == 24 ? 1.4 : .1;
        if (Now - Finished < Wait) { return false; }
        Validate();
        Receiver->RemoveFromRoot(); Receiver = nullptr; Request = nullptr; ++Stage;
        return false;
    }
private:
    void Start()
    {
        Test->AddInfo(FString::Printf(TEXT("TypeSafe stage %d"), Stage));
        Started = FPlatformTime::Seconds(); Finished = 0; bAction = false; NativeCompleted = 0; NativeFailed = 0; CommonFinished = 0;
        Receiver = NewObject<UJWNU_TypeSafeTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get()); Receiver->AddToRoot();
        State.Format = EJWNU_TypeSafeStateFormat::Json; State.Value = TEXT("{\"message\":\"편대 복귀 🙂\"}");
        Questions = {
            UJWNU_BFL_TypeSafe::MakeChoiceQuestion(TEXT("action"), TEXT("명령 선택"), {{TEXT("return"), TEXT("복귀")}, {TEXT("none"), TEXT("아님")}}),
            UJWNU_BFL_TypeSafe::MakeScoreQuestion(TEXT("priority"), TEXT("긴급도"), {TEXT("낮음"), TEXT("높음")}),
            UJWNU_BFL_TypeSafe::MakeNoulQuestion(TEXT("command"), TEXT("명령인가?"), {}) };
        if (Stage == 0)
        {
            auto* DefaultRequest = UJWNU_TypeSafeRequest::CreateTypeSafeRequest(World);
            Receiver->StartWithDefaultOptions(DefaultRequest, State, Questions);
            // 기본 endpoint·model·timeout 검증을 통과해 빈 credential 오류에 도달해야 한다. 외부 전송은 없다.
            Test->TestEqual(TEXT("Unconnected Options retains valid defaults"), DefaultRequest->GetError().Code, EJWNU_TypeSafeErrorCode::Credentials);
            Test->TestTrue(TEXT("Default-options BP actually started"), DefaultRequest->IsActive());
            DefaultRequest->Cancel();
            Receiver->CallWithDefaultOptions(World, State, Questions);
            if (Test->TestNotNull(TEXT("Immediate BP returns request"), Receiver->AuxiliaryRequest.Get()))
            {
                Test->TestEqual(TEXT("Immediate Options uses defaults"), Receiver->AuxiliaryRequest->GetError().Code, EJWNU_TypeSafeErrorCode::Credentials);
                Receiver->AuxiliaryRequest->Cancel();
            }
        }
        const TCHAR* Modes[] = {TEXT("utf8"), TEXT("401"), TEXT("422"), TEXT("retry429"), TEXT("retry529"), TEXT("malformed"),
            TEXT("missing"), TEXT("wrongtype"), TEXT("badprob"), TEXT("oversize"), TEXT("delay"), TEXT("delay"), TEXT("delay"),
            TEXT("ok"), TEXT("ok"), TEXT("delay"), TEXT("529"), TEXT("429"), TEXT("retrydate"), TEXT("retryms"), TEXT("delay"),
            TEXT("ok"), TEXT("ok"), TEXT("ok"), TEXT("delay")};
        Options = {};
        Options.Endpoint = Base + TEXT("/typesafe/v1/systemone?mode=") + Modes[Stage] + TEXT("&case=") + FGuid::NewGuid().ToString();
        Options.TimeoutSeconds = 5; Options.AttemptTimeoutSeconds = 3; Options.MaxRetries = 1;
        Options.InitialRetrySeconds = .05f; Options.MaxRetrySeconds = .1f;
        if (Stage == 9) { Options.MaxResponseBytes = 1024; }
        if (Stage == 10) { Options.TimeoutSeconds = .25f; Options.AttemptTimeoutSeconds = .1f; }
        if (Stage == 14) { Options.TimeoutSeconds = -1; }
        if (Stage == 17) { Options.MaxRetries = 0; }
        if (Stage == 21) { const auto Duplicate = Questions[0]; Questions.Add(Duplicate); }
        if (Stage == 23) { Options.Endpoint = TEXT("http://127.0.0.1.evil.example:18573/typesafe/v1/systemone"); }
        if (bImmediate)
        {
            FJWNU_TypeSafeCompletedCallback Completed; Completed.BindDynamic(Receiver, &UJWNU_TypeSafeTestReceiver::Completed);
            FJWNU_TypeSafeFailedCallback Failed; Failed.BindDynamic(Receiver, &UJWNU_TypeSafeTestReceiver::Failed);
            Request = Stage == 13
                ? UJWNU_BFL_TypeSafe::CallTypeSafeApiFromEnvironment(World, State, Questions, Options, Completed, Failed)
                : UJWNU_BFL_TypeSafe::CallTypeSafeApi(World, State, Questions, Options, Stage == 22 ? TEXT("fixture-not-a-real-key") : TEXT(""), Completed, Failed);
        }
        else { Request = UJWNU_TypeSafeRequest::CreateTypeSafeRequest(World); }
        if (!Test->TestNotNull(TEXT("Request created"), Request)) { return; }
        if (!bImmediate)
        {
            Test->TestFalse(TEXT("Create does not start the request"), Request->IsActive());
            Request->OnCompleted.AddDynamic(Receiver, &UJWNU_TypeSafeTestReceiver::Completed);
            Request->OnFailed.AddDynamic(Receiver, &UJWNU_TypeSafeTestReceiver::Failed);
        }
        Request->OnFinishedNative.AddLambda([this](UJWNU_RequestBase* Value, EJWNU_RequestState Status)
        {
            ++CommonFinished;
            Test->TestEqual(TEXT("Common callback request"), Value, static_cast<UJWNU_RequestBase*>(Request));
            Test->TestEqual(TEXT("State visible in callback"), Value->GetState(), Status);
            Test->TestEqual(TEXT("Typed callback before common callback"), Receiver->CompletedCount + Receiver->FailedCount, 1);
            Value->Cancel();
        });
        Request->OnCompletedNative.AddLambda([this](const FJWNU_TypeSafeResult&)
        {
            ++NativeCompleted;
            if (Stage == 0) { Request->Cancel(); CollectGarbage(RF_NoFlags); }
        });
        Request->OnFailedNative.AddLambda([this](const FJWNU_TypeSafeError&) { ++NativeFailed; });
        Test->TestEqual(TEXT("No completion before explicit Start"), Receiver->CompletedCount + Receiver->FailedCount, 0);
        if (!bImmediate)
        {
        const bool bScheduled = Stage == 13
            ? Request->StartFromEnvironment(State, Questions, Options)
            : Request->Start(State, Questions, Options, Stage == 22 ? TEXT("fixture-not-a-real-key") : TEXT(""));
        const bool bInvalidInput = Stage == 13 || Stage == 14 || Stage == 21 || Stage == 22 || Stage == 23;
        Test->TestEqual(TEXT("Start reports input validation"), bScheduled, !bInvalidInput);
        }
        Test->TestTrue(TEXT("Start tracks request until terminal event, including validation errors"), Request->IsActive());
        Test->TestFalse(TEXT("Second Start rejected while pending"), Request->Start(State, Questions, Options, TEXT("")));
        Test->TestEqual(TEXT("Start defers terminal events until Pump"), Receiver->CompletedCount + Receiver->FailedCount, 0);
        if (Stage == 15) { Request->Cancel(); Request->Cancel(); }
    }
    void Validate()
    {
        const bool bSuccess = Stage == 0 || Stage == 3 || Stage == 4 || Stage == 18 || Stage == 19 || Stage == 20;
        Test->TestEqual(TEXT("Exactly one BP terminal event"), Receiver->CompletedCount + Receiver->FailedCount, 1);
        Test->TestEqual(TEXT("Exactly one native terminal event"), NativeCompleted + NativeFailed, 1);
        Test->TestEqual(TEXT("Expected success"), Receiver->CompletedCount, bSuccess ? 1 : 0);
        Test->TestFalse(TEXT("Request terminal"), Request->IsActive());
        Test->TestEqual(TEXT("Common completion once"), CommonFinished, 1);
        Test->TestEqual(TEXT("Common terminal state"), Request->GetState(), bSuccess ? EJWNU_RequestState::Succeeded :
            (Receiver->LastError.Code == EJWNU_TypeSafeErrorCode::Cancelled ? EJWNU_RequestState::Cancelled : EJWNU_RequestState::Failed));
        Test->TestFalse(TEXT("Single use handle"), Request->Start(State, Questions, Options, TEXT("")));
        if (bSuccess)
        {
            Test->TestEqual(TEXT("BP true criterion forwarded"), Receiver->LastWithCriteria.YesDescription, FString(TEXT("이전 문의 있음")));
            Test->TestEqual(TEXT("BP false criterion forwarded"), Receiver->LastWithCriteria.NoDescription, FString(TEXT("이전 문의 없음")));
            Test->TestTrue(TEXT("Unconnected BP criteria remains optional"), Receiver->LastWithoutCriteria.YesDescription.IsEmpty() && Receiver->LastWithoutCriteria.NoDescription.IsEmpty());
            Test->TestEqual(TEXT("Actual model"), Receiver->LastResult.Model, FString(TEXT("fixture-jev")));
            Test->TestEqual(TEXT("Choice map via compiled BP"), Receiver->LastResult.Choices.Num(), 1);
            if (const auto* Score = Receiver->LastResult.Scores.Find(TEXT("priority"))) { Test->TestEqual(TEXT("Fractional score via BP"), Score->Score, .75); }
            else { Test->AddError(TEXT("Missing Score")); }
            if (const auto* Noul = Receiver->LastResult.Nouls.Find(TEXT("command"))) { Test->TestEqual(TEXT("Probability via BP"), Noul->Noul, .85); }
            else { Test->AddError(TEXT("Missing Noul")); }
            Test->TestEqual(TEXT("Token usage"), Receiver->LastResult.InputTokens, int64(42));
            const bool bRetry = Stage == 3 || Stage == 4 || Stage == 18 || Stage == 19;
            Test->TestEqual(TEXT("Retry count"), Receiver->LastResult.Attempts, bRetry ? 2 : 1);
            if (Stage == 3 || Stage == 4) { Test->TestTrue(TEXT("Retry-After overrides short backoff"), Finished - Started >= 1.); }
            if (Stage == 19) { Test->TestTrue(TEXT("Retry-after-ms respected"), Finished - Started >= .4); }
            return;
        }
        EJWNU_TypeSafeErrorCode Expected = EJWNU_TypeSafeErrorCode::Configuration;
        if (Stage == 1 || Stage == 2 || Stage == 16 || Stage == 17)
        {
            Expected = EJWNU_TypeSafeErrorCode::Http;
            const int32 Status = Stage == 1 ? 401 : (Stage == 2 ? 422 : (Stage == 16 ? 529 : 429));
            Test->TestEqual(TEXT("HTTP status preserved"), Receiver->LastError.HttpStatus, Status);
            Test->TestTrue(TEXT("Original error details preserved"), Receiver->LastError.ResponseBody.Contains(TEXT("fixture error preserved")));
            Test->TestEqual(TEXT("Auth/validation not retried; overload obeys budget"), Receiver->LastError.Attempts, Stage == 16 ? 2 : 1);
        }
        if (Stage >= 5 && Stage <= 8) { Expected = EJWNU_TypeSafeErrorCode::InvalidResponse; }
        if (Stage == 9) { Expected = EJWNU_TypeSafeErrorCode::ResponseLimit; }
        if (Stage == 10) { Expected = EJWNU_TypeSafeErrorCode::Timeout; }
        if (Stage == 11 || Stage == 12 || Stage == 15 || Stage == 24) { Expected = EJWNU_TypeSafeErrorCode::Cancelled; }
        if (Stage == 13) { Expected = EJWNU_TypeSafeErrorCode::Credentials; }
        Test->TestEqual(TEXT("Error category"), Receiver->LastError.Code, Expected);
    }
    void Cleanup()
    {
        if (Request) { Request->Cancel(); }
        if (!bShutdown) { Instance->Shutdown(); }
        if (Receiver) { Receiver->RemoveFromRoot(); }
        GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
        Instance->RemoveFromRoot(); BP->RemoveFromRoot();
    }
    FAutomationTestBase* Test;
    UGameInstance* Instance = nullptr;
    UWorld* World = nullptr;
    UBlueprint* BP = nullptr;
    UJWNU_TypeSafeRequest* Request = nullptr;
    UJWNU_TypeSafeTestReceiver* Receiver = nullptr;
    FJWNU_TypeSafeState State;
    FJWNU_TypeSafeOptions Options;
    TArray<FJWNU_TypeSafeQuestion> Questions;
    FString Base;
    int32 Stage = 0, NativeCompleted = 0, NativeFailed = 0;
    double Started = 0, Finished = 0;
    bool bAction = false, bShutdown = false;
    bool bImmediate = false;
    int32 CommonFinished = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_TypeSafeIntegrationTest, "JWNetworkUtility.TypeSafe.FastAPI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_TypeSafeIntegrationTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUTypeSafeIntegration")))
    { AddInfo(TEXT("Skipped; use TestServer/run_typesafe_tests.py to start the fixture.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::TypeSafeTest::FIntegration(this));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_TypeSafeImmediateTest, "JWNetworkUtility.TypeSafe.Immediate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_TypeSafeImmediateTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UJWNU_TypeSafeTestReceiver> Receiver(NewObject<UJWNU_TypeSafeTestReceiver>());
    FJWNU_TypeSafeFailedCallback Failed; Failed.BindDynamic(Receiver.Get(), &UJWNU_TypeSafeTestReceiver::Failed);
    TestNull(TEXT("Invalid world immediate"), UJWNU_BFL_TypeSafe::CallTypeSafeApi(nullptr, {}, {}, {}, TEXT(""), {}, Failed));
    TestNull(TEXT("Invalid world environment"), UJWNU_BFL_TypeSafe::CallTypeSafeApiFromEnvironment(nullptr, {}, {}, {}, {}, Failed));
    TestEqual(TEXT("One failure per invalid world call"), Receiver->FailedCount, 2);
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUTypeSafeIntegration")))
    { AddInfo(TEXT("Skipped; use TestServer/run_typesafe_tests.py to start the fixture.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::TypeSafeTest::FIntegration(this, true));
    return true;
}
#endif
