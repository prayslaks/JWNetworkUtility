// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_SystemOneTestReceiver.h"
#include "JWNU_SystemOneRequest.h"
#include "JWNU_BFL_SystemOne.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/GarbageCollection.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "JWNU_SystemOneTestHelpers.h"

namespace JWNU::SystemOneTest
{

class FIntegration : public IAutomationLatentCommand
{
public:
    FIntegration(FAutomationTestBase* InTest, bool bInImmediate) : Test(InTest), bImmediate(bInImmediate) {}
    virtual bool Update() override
    {
        if (!Instance)
        {
            FParse::Value(FCommandLine::Get(), TEXT("JWNUSystemOneTestURL="), Base);
            Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone();
            World = Instance->GetWorld(); BP = BuildReceiver(Test); BP->AddToRoot();
        }
        if (Stage == 8) { Cleanup(); return true; }
        if (!Receiver) { Start(); }
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 8) { Test->AddError(TEXT("System One fixture timed out")); Cleanup(); return true; }
        if (!bAction && Now - Started > .15)
        {
            bAction = true;
            if (Stage == 5) { CollectGarbage(RF_NoFlags); Test->TestTrue(TEXT("Active request survives GC"), IsValid(Request) && Request->IsActive()); }
            if (Stage == 6) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); }
            if (Stage == 7) { Instance->Shutdown(); bShutdown = true; }
        }
        if (Receiver->CompletedCount + Receiver->FailedCount == 0) { return false; }
        if (!Finished) { Finished = Now; return false; }
        if (Now - Finished < (Stage >= 6 ? 1.4 : .1)) { return false; }
        Test->TestEqual(TEXT("One typed event"), Receiver->CompletedCount + Receiver->FailedCount, 1);
        Test->TestEqual(TEXT("One common event"), FinishedCount, 1);
        const bool bSuccess = Stage < 3 || Stage == 5;
        Test->TestEqual(TEXT("Expected outcome"), Receiver->CompletedCount, bSuccess ? 1 : 0);
        Test->TestFalse(TEXT("Request is terminal"), Request->IsActive());
        if (bSuccess && Receiver->CompletedCount == 1)
        {
            const auto& Result = Receiver->LastResult;
            Test->TestEqual(TEXT("Model ID reaches server and BP"), Result.Model, Models[Stage % 3]);
            Test->TestEqual(TEXT("Request ID retained"), Result.RequestId, FString(TEXT("fixture-decision")));
            Test->TestEqual(TEXT("Provider retained"), Result.Provider, FString(TEXT("FixtureProvider")));
            Test->TestTrue(TEXT("Cost retained"), Result.bHasCost && FMath::IsNearlyEqual(Result.Cost, .00001));
            Test->TestEqual(TEXT("All question types decoded"), Result.Choices.Num() + Result.Scores.Num() + Result.Nouls.Num(), 3);
        }
        else if (!bSuccess) { Test->TestEqual(TEXT("Error category"), Receiver->LastError.Code, Stage == 3 ? EJWNU_SystemOneErrorCode::InvalidResponse : EJWNU_SystemOneErrorCode::Cancelled); }
        Receiver->RemoveFromRoot(); Receiver = nullptr; Request = nullptr; ++Stage;
        return false;
    }
private:
    void Start()
    {
        Started = FPlatformTime::Seconds(); Finished = 0; FinishedCount = 0; bAction = false;
        Receiver = NewObject<UJWNU_SystemOneTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get()); Receiver->AddToRoot();
        FJWNU_SystemOneState State; State.Format = EJWNU_SystemOneStateFormat::Json; State.Value = TEXT("{\"message\":\"편대 복귀 🙂\"}");
        const TArray<FJWNU_SystemOneQuestion> Questions {
            UJWNU_BFL_SystemOne::MakeChoiceQuestion(TEXT("action"), TEXT("명령"), {{TEXT("return"), TEXT("복귀")}, {TEXT("none"), TEXT("아님")}}),
            UJWNU_BFL_SystemOne::MakeScoreQuestion(TEXT("priority"), TEXT("긴급도"), {TEXT("낮음"), TEXT("높음")}),
            UJWNU_BFL_SystemOne::MakeNoulQuestion(TEXT("command"), TEXT("명령인가?"), {}) };
        auto Options = UJWNU_BFL_SystemOne::MakeOpenRouterOptions(Models[Stage % 3]);
        Options.Endpoint = Base + TEXT("/systemone/decisions?mode=") + (Stage == 3 ? TEXT("badprob") : (Stage >= 5 ? TEXT("delay") : TEXT("utf8")));
        if (Stage == 0)
        {
            auto* Defaults = UJWNU_SystemOneRequest::CreateSystemOneRequest(World);
            Receiver->StartDefaults(Defaults, State, Questions);
            Test->TestEqual(TEXT("BP default options reaches missing key check"), Defaults->GetError().Code, EJWNU_SystemOneErrorCode::Credentials);
            Defaults->Cancel();
            // 별도 주소는 환경변수를 읽기 전에 거절한다. 실제 계정 키·외부 전송은 사용하지 않는다.
            for (const FString& Endpoint : {Options.Endpoint, FString(TEXT("https://openrouter.ai.evil.example/api/alpha/decisions")), FString(TEXT("https://openrouter.ai/api/alpha/decisions?relay=1"))})
            {
                auto* Rejected = UJWNU_SystemOneRequest::CreateSystemOneRequest(World);
                auto Unsafe = Options; Unsafe.Endpoint = Endpoint;
                Test->TestFalse(TEXT("Environment credentials restricted to official URL"), Rejected->StartFromEnvironment(State, Questions, Unsafe));
                Test->TestEqual(TEXT("Environment rejection category"), Rejected->GetError().Code, EJWNU_SystemOneErrorCode::Credentials);
                Rejected->Cancel();
            }
        }
        if (bImmediate)
        {
            FJWNU_SystemOneCompletedCallback Completed; Completed.BindDynamic(Receiver, &UJWNU_SystemOneTestReceiver::Completed);
            FJWNU_SystemOneFailedCallback Failed; Failed.BindDynamic(Receiver, &UJWNU_SystemOneTestReceiver::Failed);
            Request = UJWNU_BFL_SystemOne::CallSystemOneApi(World, State, Questions, Options, TEXT(""), Completed, Failed);
        }
        else
        {
            Request = UJWNU_SystemOneRequest::CreateSystemOneRequest(World);
            Request->OnCompleted.AddDynamic(Receiver, &UJWNU_SystemOneTestReceiver::Completed);
            Request->OnFailed.AddDynamic(Receiver, &UJWNU_SystemOneTestReceiver::Failed);
            Test->TestTrue(TEXT("Request schedules"), Request->Start(State, Questions, Options, TEXT("")));
        }
        Request->OnFinishedNative.AddLambda([this](UJWNU_RequestBase* Value, EJWNU_RequestState Status)
        {
            ++FinishedCount;
            Test->TestEqual(TEXT("Terminal state visible"), Value->GetState(), Status);
            Test->TestEqual(TEXT("Typed event precedes common event"), Receiver->CompletedCount + Receiver->FailedCount, 1);
            Value->Cancel();
        });
        Test->TestEqual(TEXT("No synchronous completion"), Receiver->CompletedCount + Receiver->FailedCount, 0);
        Test->TestFalse(TEXT("Duplicate start rejected"), Request->Start(State, Questions, Options, TEXT("")));
        if (Stage == 4) { Request->Cancel(); Request->Cancel(); }
    }
    void Cleanup()
    {
        if (Request) { Request->Cancel(); }
        if (!bShutdown) { Instance->Shutdown(); }
        if (Receiver) { Receiver->RemoveFromRoot(); }
        GEngine->DestroyWorldContext(World); World->DestroyWorld(false); Instance->RemoveFromRoot(); BP->RemoveFromRoot();
    }
    FAutomationTestBase* Test;
    bool bImmediate = false, bAction = false, bShutdown = false;
    UGameInstance* Instance = nullptr;
    UWorld* World = nullptr;
    UBlueprint* BP = nullptr;
    UJWNU_SystemOneRequest* Request = nullptr;
    UJWNU_SystemOneTestReceiver* Receiver = nullptr;
    FString Base = TEXT("http://127.0.0.1:18573");
    TArray<FString> Models = UJWNU_BFL_SystemOne::GetOpenRouterModelPresets();
    int32 Stage = 0, FinishedCount = 0;
    double Started = 0, Finished = 0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_SystemOneIntegrationTest, "JWNetworkUtility.SystemOne.FastAPI", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_SystemOneIntegrationTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUSystemOneIntegration"))) { AddInfo(TEXT("Use run_systemone_tests.py for local HTTP fixtures.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::SystemOneTest::FIntegration(this, false)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_SystemOneImmediateTest, "JWNetworkUtility.SystemOne.Immediate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_SystemOneImmediateTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUSystemOneIntegration"))) { AddInfo(TEXT("Use run_systemone_tests.py for local HTTP fixtures.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::SystemOneTest::FIntegration(this, true)); return true;
}
#endif
