// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_SystemOneTransportTestReceiver.h"
#include "JWNU_SystemOneRequest.h"
#include "JWNU_BFL_SystemOne.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "JWNU_SystemOneTransportTestHelpers.h"

namespace JWNU::SystemOneTransportTest
{
class FIntegration : public IAutomationLatentCommand
{
public:
    explicit FIntegration(FAutomationTestBase* InTest, bool bInImmediate = false) : Test(InTest), bImmediate(bInImmediate) {}
    virtual bool Update() override
    {
        if (!Instance)
        {
            Base = TEXT("http://127.0.0.1:18573");
            FParse::Value(FCommandLine::Get(), TEXT("JWNUSystemOneTestURL="), Base);
            Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone();
            World = Instance->GetWorld(); BP = BuildReceiver(Test); BP->AddToRoot();
        }
        if (Stage == 25) { Cleanup(); return true; }
        if (!Receiver) { Start(); }
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 8)
        { Test->AddError(FString::Printf(TEXT("SystemOne stage %d timed out"), Stage)); Cleanup(); return true; }
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
        Test->AddInfo(FString::Printf(TEXT("SystemOne stage %d"), Stage));
        Started = FPlatformTime::Seconds(); Finished = 0; bAction = false; NativeCompleted = 0; NativeFailed = 0; CommonFinished = 0;
        Receiver = NewObject<UJWNU_SystemOneTransportTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get()); Receiver->AddToRoot();
        State.Format = EJWNU_SystemOneStateFormat::Json; State.Value = TEXT("{\"message\":\"편대 복귀 🙂\"}");
        Questions = {
            UJWNU_BFL_SystemOne::MakeChoiceQuestion(TEXT("action"), TEXT("명령 선택"), {{TEXT("return"), TEXT("복귀")}, {TEXT("none"), TEXT("아님")}}),
            UJWNU_BFL_SystemOne::MakeScoreQuestion(TEXT("priority"), TEXT("긴급도"), {TEXT("낮음"), TEXT("높음")}),
            UJWNU_BFL_SystemOne::MakeNoulQuestion(TEXT("command"), TEXT("명령인가?"), {}) };
        if (Stage == 0)
        {
            auto* DefaultRequest = UJWNU_SystemOneRequest::CreateSystemOneRequest(World);
            Receiver->StartWithDefaultOptions(DefaultRequest, State, Questions);
            // 기본 endpoint·model·timeout 검증을 통과해 빈 credential 오류에 도달해야 한다. 외부 전송은 없다.
            Test->TestEqual(TEXT("Unconnected Options retains valid defaults"), DefaultRequest->GetError().Code, EJWNU_SystemOneErrorCode::Credentials);
            Test->TestTrue(TEXT("Default-options BP actually started"), DefaultRequest->IsActive());
            DefaultRequest->Cancel();
            Receiver->CallWithDefaultOptions(World, State, Questions);
            if (Test->TestNotNull(TEXT("Immediate BP returns request"), Receiver->AuxiliaryRequest.Get()))
            {
                Test->TestEqual(TEXT("Immediate Options uses defaults"), Receiver->AuxiliaryRequest->GetError().Code, EJWNU_SystemOneErrorCode::Credentials);
                Receiver->AuxiliaryRequest->Cancel();
            }
        }
        const TCHAR* Modes[] = {TEXT("utf8"), TEXT("401"), TEXT("422"), TEXT("retry429"), TEXT("retry529"), TEXT("malformed"),
            TEXT("missing"), TEXT("wrongtype"), TEXT("badprob"), TEXT("oversize"), TEXT("delay"), TEXT("delay"), TEXT("delay"),
            TEXT("ok"), TEXT("ok"), TEXT("delay"), TEXT("529"), TEXT("429"), TEXT("retrydate"), TEXT("retryms"), TEXT("delay"),
            TEXT("ok"), TEXT("ok"), TEXT("ok"), TEXT("delay")};
        Options = UJWNU_BFL_SystemOne::MakeTypeSafeOptions();
        Options.Endpoint = Base + TEXT("/systemone/direct?mode=") + Modes[Stage] + TEXT("&case=") + FGuid::NewGuid().ToString();
        Options.TimeoutSeconds = 5; Options.AttemptTimeoutSeconds = 3; Options.MaxRetries = 1;
        Options.InitialRetrySeconds = .05f; Options.MaxRetrySeconds = .1f;
        if (Stage == 9) { Options.MaxResponseBytes = 1024; }
        if (Stage == 10) { Options.TimeoutSeconds = .25f; Options.AttemptTimeoutSeconds = .1f; }
        if (Stage == 14) { Options.TimeoutSeconds = -1; }
        if (Stage == 17) { Options.MaxRetries = 0; }
        if (Stage == 21) { const auto Duplicate = Questions[0]; Questions.Add(Duplicate); }
        if (Stage == 23) { Options.Endpoint = TEXT("http://127.0.0.1.evil.example:18573/systemone/direct"); }
        if (bImmediate)
        {
            FJWNU_SystemOneCompletedCallback Completed; Completed.BindDynamic(Receiver, &UJWNU_SystemOneTransportTestReceiver::Completed);
            FJWNU_SystemOneFailedCallback Failed; Failed.BindDynamic(Receiver, &UJWNU_SystemOneTransportTestReceiver::Failed);
            Request = Stage == 13
                ? UJWNU_BFL_SystemOne::CallSystemOneApiFromEnvironment(World, State, Questions, Options, Completed, Failed)
                : UJWNU_BFL_SystemOne::CallSystemOneApi(World, State, Questions, Options, Stage == 22 ? TEXT("fixture-not-a-real-key") : TEXT(""), Completed, Failed);
        }
        else { Request = UJWNU_SystemOneRequest::CreateSystemOneRequest(World); }
        if (!Test->TestNotNull(TEXT("Request created"), Request)) { return; }
        if (!bImmediate)
        {
            Test->TestFalse(TEXT("Create does not start the request"), Request->IsActive());
            Request->OnCompleted.AddDynamic(Receiver, &UJWNU_SystemOneTransportTestReceiver::Completed);
            Request->OnFailed.AddDynamic(Receiver, &UJWNU_SystemOneTransportTestReceiver::Failed);
        }
        Request->OnFinishedNative.AddLambda([this](UJWNU_RequestBase* Value, EJWNU_RequestState Status)
        {
            ++CommonFinished;
            Test->TestEqual(TEXT("Common callback request"), Value, static_cast<UJWNU_RequestBase*>(Request));
            Test->TestEqual(TEXT("State visible in callback"), Value->GetState(), Status);
            Test->TestEqual(TEXT("Typed callback before common callback"), Receiver->CompletedCount + Receiver->FailedCount, 1);
            Value->Cancel();
        });
        Request->OnCompletedNative.AddLambda([this](const FJWNU_SystemOneResult&)
        {
            ++NativeCompleted;
            if (Stage == 0) { Request->Cancel(); CollectGarbage(RF_NoFlags); }
        });
        Request->OnFailedNative.AddLambda([this](const FJWNU_SystemOneError&) { ++NativeFailed; });
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
            (Receiver->LastError.Code == EJWNU_SystemOneErrorCode::Cancelled ? EJWNU_RequestState::Cancelled : EJWNU_RequestState::Failed));
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
        EJWNU_SystemOneErrorCode Expected = EJWNU_SystemOneErrorCode::Configuration;
        if (Stage == 1 || Stage == 2 || Stage == 16 || Stage == 17)
        {
            Expected = EJWNU_SystemOneErrorCode::Http;
            const int32 Status = Stage == 1 ? 401 : (Stage == 2 ? 422 : (Stage == 16 ? 529 : 429));
            Test->TestEqual(TEXT("HTTP status preserved"), Receiver->LastError.HttpStatus, Status);
            Test->TestTrue(TEXT("Original error details preserved"), Receiver->LastError.ResponseBody.Contains(TEXT("fixture error preserved")));
            Test->TestEqual(TEXT("Auth/validation not retried; overload obeys budget"), Receiver->LastError.Attempts, Stage == 16 ? 2 : 1);
        }
        if (Stage >= 5 && Stage <= 8) { Expected = EJWNU_SystemOneErrorCode::InvalidResponse; }
        if (Stage == 9) { Expected = EJWNU_SystemOneErrorCode::ResponseLimit; }
        if (Stage == 10) { Expected = EJWNU_SystemOneErrorCode::Timeout; }
        if (Stage == 11 || Stage == 12 || Stage == 15 || Stage == 24) { Expected = EJWNU_SystemOneErrorCode::Cancelled; }
        if (Stage == 13) { Expected = EJWNU_SystemOneErrorCode::Credentials; }
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
    UJWNU_SystemOneRequest* Request = nullptr;
    UJWNU_SystemOneTransportTestReceiver* Receiver = nullptr;
    FJWNU_SystemOneState State;
    FJWNU_SystemOneOptions Options;
    TArray<FJWNU_SystemOneQuestion> Questions;
    FString Base;
    int32 Stage = 0, NativeCompleted = 0, NativeFailed = 0;
    double Started = 0, Finished = 0;
    bool bAction = false, bShutdown = false;
    bool bImmediate = false;
    int32 CommonFinished = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_SystemOneTransportIntegrationTest, "JWNetworkUtility.SystemOne.Transport.FastAPI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_SystemOneTransportIntegrationTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUSystemOneIntegration")))
    { AddInfo(TEXT("Skipped; use TestServer/run_systemone_tests.py to start the fixture.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::SystemOneTransportTest::FIntegration(this));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_SystemOneTransportImmediateTest, "JWNetworkUtility.SystemOne.Transport.Immediate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_SystemOneTransportImmediateTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UJWNU_SystemOneTransportTestReceiver> Receiver(NewObject<UJWNU_SystemOneTransportTestReceiver>());
    FJWNU_SystemOneFailedCallback Failed; Failed.BindDynamic(Receiver.Get(), &UJWNU_SystemOneTransportTestReceiver::Failed);
    TestNull(TEXT("Invalid world immediate"), UJWNU_BFL_SystemOne::CallSystemOneApi(nullptr, {}, {}, {}, TEXT(""), {}, Failed));
    TestNull(TEXT("Invalid world environment"), UJWNU_BFL_SystemOne::CallSystemOneApiFromEnvironment(nullptr, {}, {}, {}, {}, Failed));
    TestEqual(TEXT("One failure per invalid world call"), Receiver->FailedCount, 2);
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUSystemOneIntegration")))
    { AddInfo(TEXT("Skipped; use TestServer/run_systemone_tests.py to start the fixture.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::SystemOneTransportTest::FIntegration(this, true));
    return true;
}
#endif
