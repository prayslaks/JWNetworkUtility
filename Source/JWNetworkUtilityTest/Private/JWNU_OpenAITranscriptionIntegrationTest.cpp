// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_OpenAITranscriptionTestReceiver.h"
#include "JWNU_OpenAITranscriptionSession.h"
#include "JWNU_OpenAITranscriptionComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
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

namespace JWNU::OpenAITranscriptionTest
{
inline UBlueprint* BuildReceiver(FAutomationTestBase* Test)
{
    auto* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_OpenAITranscriptionTestReceiver::StaticClass(), GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_TranscriptionAutomation")),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("ReceiveGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
    auto* Event = NewObject<UK2Node_Event>(Graph);
    Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UJWNU_OpenAITranscriptionTestReceiver, Transcript), UJWNU_OpenAITranscriptionTestReceiver::StaticClass());
    Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
    auto* Record = NewObject<UK2Node_CallFunction>(Graph);
    Record->SetFromFunction(UJWNU_OpenAITranscriptionTestReceiver::StaticClass()->FindFunctionByName(TEXT("Record")));
    Graph->AddNode(Record); Record->CreateNewGuid(); Record->AllocateDefaultPins();
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    for (int32 Variant = 0; Variant < 4; ++Variant)
    {
        const TCHAR* EventNames[] = {TEXT("StartSessionDefaults"), TEXT("StartSessionEnvironmentDefaults"), TEXT("StartComponentDefaults"), TEXT("StartComponentEnvironmentDefaults")};
        auto* StartEvent = NewObject<UK2Node_Event>(Graph);
        StartEvent->EventReference.SetExternalMember(EventNames[Variant], UJWNU_OpenAITranscriptionTestReceiver::StaticClass());
        StartEvent->bOverrideFunction = true; Graph->AddNode(StartEvent); StartEvent->CreateNewGuid(); StartEvent->AllocateDefaultPins();
        auto* TargetClass = Variant < 2 ? UJWNU_OpenAITranscriptionSession::StaticClass() : UJWNU_OpenAITranscriptionComponent::StaticClass();
        auto* StartCall = NewObject<UK2Node_CallFunction>(Graph);
        StartCall->SetFromFunction(TargetClass->FindFunctionByName(Variant % 2 == 0 ? TEXT("Start") : TEXT("StartFromEnvironment")));
        Graph->AddNode(StartCall); StartCall->CreateNewGuid(); StartCall->AllocateDefaultPins();
        Test->TestTrue(TEXT("Live default Options exec"), Schema->TryCreateConnection(StartEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        Test->TestTrue(TEXT("Live default Options target"), Schema->TryCreateConnection(StartEvent->FindPinChecked(TEXT("Target")), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Self)));
        Test->TestTrue(TEXT("Live Options intentionally unconnected"), StartCall->FindPinChecked(TEXT("Options"))->LinkedTo.IsEmpty());
        Test->TestFalse(TEXT("Live Options default is used"), StartCall->FindPinChecked(TEXT("Options"))->bDefaultValueIsIgnored);
    }
    Test->TestTrue(TEXT("BP transcript wire"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Value")), Record->FindPinChecked(TEXT("Value"))));
    Test->TestTrue(TEXT("BP exec wire"), Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Record->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
    FKismetEditorUtilities::CompileBlueprint(BP);
    Test->TestTrue(TEXT("Live receiver BP compiled"), BP->Status != BS_Error);
    return BP;
}




class FIntegration : public IAutomationLatentCommand
{
public:
    explicit FIntegration(FAutomationTestBase* InTest) : Test(InTest) {}
    virtual bool Update() override
    {
        if (!Instance)
        {
            Base = TEXT("ws://127.0.0.1:18573");
            FParse::Value(FCommandLine::Get(), TEXT("JWNUTranscriptionTestURL="), Base);
            Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone();
            World = Instance->GetWorld();
            BP = BuildReceiver(Test); BP->AddToRoot();
            Payload.SetNumZeroed(4800);
            TStrongObjectPtr<UJWNU_OpenAITranscriptionTestReceiver> Defaults(NewObject<UJWNU_OpenAITranscriptionTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get()));
            auto* DefaultSession = UJWNU_OpenAITranscriptionSession::CreateOpenAITranscriptionSession(World);
            DefaultSession->OnError.AddDynamic(Defaults.Get(), &UJWNU_OpenAITranscriptionTestReceiver::Error);
            Defaults->StartSessionDefaults(DefaultSession);
            Test->TestEqual(TEXT("Unconnected session Options execute in BP"), Defaults->LastError.Code, FString(TEXT("credentials")));
            auto* Owner = World->SpawnActor<AActor>();
            auto* DefaultComponent = NewObject<UJWNU_OpenAITranscriptionComponent>(Owner);
            DefaultComponent->bUseMicrophone = false; DefaultComponent->RegisterComponent();
            DefaultComponent->OnError.AddDynamic(Defaults.Get(), &UJWNU_OpenAITranscriptionTestReceiver::Error);
            Defaults->StartComponentDefaults(DefaultComponent);
            Test->TestEqual(TEXT("Unconnected component Options execute in BP"), Defaults->ErrorCount, 2);
            DefaultComponent->DestroyComponent(); Owner->Destroy();
        }
        if (Stage == 21) { Cleanup(); return true; }
        if (!Receiver) { Start(); }
        if (FPlatformTime::Seconds() - Started > 6)
        { Test->AddError(FString::Printf(TEXT("Transcription stage %d timeout"), Stage)); Cleanup(); return true; }
        if (!bSent && Session->GetState() == EJWNU_OpenAITranscriptionState::Ready)
        {
            bSent = true;
            CollectGarbage(RF_NoFlags);
            Test->TestTrue(TEXT("Active session retained through GC"), Session->IsActive());
            if (Stage == 15) { Component->DestroyComponent(); }
            else if (Stage == 16) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); }
            else if (Stage == 20) { Instance->Shutdown(); bShutdown = true; }
            else if (Stage == 3) { Test->TestTrue(TEXT("Close without input"), Session->Close()); }
            else if (Stage == 0 || Stage == 1 || Stage == 2 || Stage == 4 || Stage == 5 || Stage == 8 || Stage == 13 || Stage == 19)
            {
                Test->TestFalse(TEXT("Empty commit rejected"), Session->CommitInputAudio());
                Test->TestFalse(TEXT("Odd PCM rejected"), Session->AppendInputAudio({1}));
                TArray<uint8> Oversize; Oversize.SetNumZeroed(4802);
                Test->TestFalse(TEXT("Chunk over 100ms rejected"), Session->AppendInputAudio(Oversize));
                if (Stage == 2)
                {
                    TArray<uint8> Short; Short.SetNumZeroed(960);
                    Test->TestTrue(TEXT("20ms tail accepted"), Session->AppendInputAudio(Short));
                    Test->TestFalse(TEXT("Too short manual commit rejected"), Session->CommitInputAudio());
                }
                else
                {
                    Test->TestTrue(TEXT("100ms PCM accepted"), Session->AppendInputAudio(Payload));
                    if (Stage == 4)
                    {
                        Test->TestTrue(TEXT("Uncommitted audio cleared"), Session->ClearInputAudio());
                        Test->TestFalse(TEXT("Cleared buffer cannot commit"), Session->CommitInputAudio());
                    }
                    else
                    {
                        Test->TestTrue(TEXT("Turn commit accepted"), Component ? Component->CommitInputAudio() : Session->CommitInputAudio());
                        if (Stage == 1)
                        {
                            Test->TestTrue(TEXT("Second turn audio accepted"), Session->AppendInputAudio(Payload));
                            Test->TestTrue(TEXT("Second turn commit accepted"), Session->CommitInputAudio());
                        }
                    }
                }
                if (Stage != 19)
                {
                    if (Component) { Component->Close(); }
                    else { Test->TestTrue(TEXT("Close drains all pending turns"), Session->Close()); }
                    Test->TestFalse(TEXT("No append after Close"), Session->AppendInputAudio(Payload));
                    Test->TestFalse(TEXT("Duplicate Close rejected"), Session->Close());
                }
            }
        }
        if (Receiver->ClosedCount == 0) { return false; }
        if (!Finished) { Finished = FPlatformTime::Seconds(); return false; }
        if (FPlatformTime::Seconds() - Finished < .05) { return false; }
        Validate();
        if (Actor) { Actor->Destroy(); Actor = nullptr; Component = nullptr; }
        Receiver->RemoveFromRoot(); Receiver = nullptr; Session = nullptr;
        ++Stage; return false;
    }
private:
    void Start()
    {
        Started = FPlatformTime::Seconds(); Finished = 0; bSent = false; NativeClosed = 0; ReadyBeforeUpdated = false;
        Receiver = NewObject<UJWNU_OpenAITranscriptionTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get()); Receiver->AddToRoot();
        const TCHAR* Modes[] = {TEXT("normal"), TEXT("reverse"), TEXT("normal"), TEXT("normal"), TEXT("normal"), TEXT("failed_item"),
            TEXT("reject"), TEXT("start_timeout"), TEXT("close_timeout"), TEXT("bad_format"), TEXT("bad_model"), TEXT("malformed"),
            TEXT("drop"), TEXT("commit_error"), TEXT("normal"), TEXT("normal"), TEXT("normal"), TEXT("normal"), TEXT("normal"), TEXT("normal"), TEXT("normal")};
        FJWNU_OpenAITranscriptionOptions Options;
        Options.Endpoint = Base + TEXT("/realtime?intent=transcription&mode=") + Modes[Stage];
        Options.StartTimeoutSeconds = Stage == 7 ? .2f : 3.f;
        Options.CloseTimeoutSeconds = Stage == 8 ? .2f : 3.f;
        Options.Delay = EJWNU_OpenAITranscriptionDelay::Low;
        namespace Models = JWNU::OpenAITranscription::Models;
        if (Stage == 1 || Stage == 2)
        {
            Options.Model = Stage == 1 ? Models::LiveTranscribe : Models::Transcribe;
            Options.Languages = {TEXT("ko"), TEXT("en")}; Options.Keywords = {TEXT("ProjectZK")}; Options.Prompt = TEXT("비행 명령");
        }
        else { Options.Model = Models::RealtimeWhisper; Options.Language = TEXT("ko"); }
        if (Stage == 0 || Stage == 15)
        {
            Actor = World->SpawnActor<AActor>(); Component = NewObject<UJWNU_OpenAITranscriptionComponent>(Actor);
            Component->bUseMicrophone = false; Component->RegisterComponent();
            Component->OnReady.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Ready);
            Component->OnTranscript.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Transcript);
            Component->OnCommitted.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Committed);
            Component->OnError.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Error);
            Component->OnClosed.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Closed);
            Test->TestTrue(TEXT("Component Start accepted"), Component->Start(Options, TEXT("")));
            Session = Component->GetSession();
        }
        else
        {
            Session = UJWNU_OpenAITranscriptionSession::CreateOpenAITranscriptionSession(World);
            Session->OnReady.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Ready);
            Session->OnTranscript.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Transcript);
            Session->OnCommitted.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Committed);
            Session->OnError.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Error);
            Session->OnClosed.AddDynamic(Receiver, &UJWNU_OpenAITranscriptionTestReceiver::Closed);
        }
        Session->OnClosedNative.AddLambda([this](const FJWNU_OpenAITranscriptionClose&) { ++NativeClosed; });
        Session->OnRawEventNative.AddLambda([this](const FString& Type, const FString&)
        {
            if (Type == TEXT("session.created")) { ReadyBeforeUpdated = Session->GetState() == EJWNU_OpenAITranscriptionState::Ready; }
        });
        if (Stage == 19)
        {
            Session->OnTranscriptNative.AddLambda([this](const FJWNU_OpenAITranscript& Value)
            { if (Value.bFinal) { Session->Close(); } });
        }
        if (Stage == 17) { Test->TestFalse(TEXT("Environment key cannot go to custom endpoint"), Session->StartFromEnvironment(Options)); }
        else if (Stage == 18)
        {
            // 모델별 필드 지원 여부는 클라이언트가 아니라 서버가 판정한다.
            Options.Prompt = TEXT("unsupported Whisper context");
            Test->TestTrue(TEXT("Cross-model fields reach the server"), Session->Start(Options, TEXT("")));
        }
        else if (!Component) { Test->TestTrue(TEXT("Session Start accepted"), Session->Start(Options, TEXT(""))); }
        Test->TestFalse(TEXT("Audio before Ready rejected"), Session->AppendInputAudio(Payload));
        if (Stage == 14) { Session->Cancel(); Session->Cancel(); }
    }
    void Validate()
    {
        Test->AddInfo(FString::Printf(TEXT("Validated transcription scenario %d"), Stage));
        const bool bFatal = (Stage >= 6 && Stage <= 13) || Stage == 17 || Stage == 18;
        const bool bFinalized = Stage <= 4 || Stage == 19;
        Test->TestEqual(TEXT("One BP terminal event"), Receiver->ClosedCount, 1);
        Test->TestEqual(TEXT("One native terminal event"), NativeClosed, 1);
        Test->TestEqual(TEXT("Expected error count"), Receiver->ErrorCount, (bFatal || Stage == 5) ? 1 : 0);
        Test->TestEqual(TEXT("Finalized only after successful draining"), Receiver->LastClose.bFinalized, bFinalized);
        Test->TestFalse(TEXT("session.created is not Ready"), ReadyBeforeUpdated);
        Test->TestFalse(TEXT("Terminal session inactive"), Session->IsActive());
        Test->TestFalse(TEXT("Single-use session"), Session->Start({}, TEXT("")));
        if (bFatal) { Test->TestTrue(TEXT("Fatal error"), Receiver->LastError.bFatal); }
        if (Stage == 5)
        {
            Test->TestFalse(TEXT("Item failure is nonfatal"), Receiver->LastError.bFatal);
            Test->TestEqual(TEXT("Failed item correlation"), Receiver->LastError.ItemId, FString(TEXT("item_1")));
        }
        if (Stage == 18) { Test->TestEqual(TEXT("Server rejected model-specific field"), Receiver->LastError.Code, FString(TEXT("invalid_session"))); }
        if (Stage == 6 || Stage == 13) { Test->TestFalse(TEXT("Server error event ID preserved"), Receiver->LastError.EventId.IsEmpty()); }
        if (Stage == 7 || Stage == 8) { Test->TestEqual(TEXT("Deadline enforced"), Receiver->LastError.Code, FString(TEXT("timeout"))); }
        if (Stage == 0 || Stage == 1 || Stage == 2 || Stage == 19)
        {
            TArray<FString> Finals;
            for (const auto& Transcript : Receiver->Transcripts)
            {
                if (Transcript.bFinal)
                {
                    Finals.Add(Transcript.ItemId);
                    Test->TestEqual(TEXT("Final text replaces partial"), Transcript.Transcript, TEXT("안녕 🙂 최종 ") + Transcript.ItemId);
                    Test->TestTrue(TEXT("Final is not a delta"), Transcript.Delta.IsEmpty());
                }
                else { Test->TestEqual(TEXT("UTF8 partial preserved"), Transcript.Delta, FString(TEXT("안녕 🙂 "))); }
            }
            Test->TestEqual(TEXT("All final transcripts reach compiled Blueprint"), Finals.Num(), Stage == 1 ? 2 : 1);
            Test->TestEqual(TEXT("Commit acknowledgements"), Receiver->Commits.Num(), Stage == 1 ? 2 : 1);
            if (Stage == 1 && Finals.Num() == 2 && Receiver->Commits.Num() == 2)
            {
                Test->TestEqual(TEXT("Out of order final preserved"), Finals[0], FString(TEXT("item_2")));
                Test->TestEqual(TEXT("Previous item maintains utterance order"), Receiver->Commits[1].PreviousItemId, FString(TEXT("item_1")));
            }
        }
        if (Stage == 4) { Test->TestEqual(TEXT("Cleared turn was not committed"), Receiver->Commits.Num(), 0); }
    }
    void Cleanup()
    {
        if (Session) { Session->Cancel(); }
        if (Actor) { Actor->Destroy(); }
        if (!bShutdown) { Instance->Shutdown(); }
        if (Receiver) { Receiver->RemoveFromRoot(); }
        GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
        Instance->RemoveFromRoot(); BP->RemoveFromRoot();
    }
    FAutomationTestBase* Test;
    UGameInstance* Instance = nullptr;
    UWorld* World = nullptr;
    UBlueprint* BP = nullptr;
    AActor* Actor = nullptr;
    UJWNU_OpenAITranscriptionComponent* Component = nullptr;
    UJWNU_OpenAITranscriptionSession* Session = nullptr;
    UJWNU_OpenAITranscriptionTestReceiver* Receiver = nullptr;
    FString Base;
    TArray<uint8> Payload;
    int32 Stage = 0, NativeClosed = 0;
    double Started = 0, Finished = 0;
    bool bSent = false, bShutdown = false, ReadyBeforeUpdated = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_OpenAITranscriptionIntegrationTest, "JWNetworkUtility.OpenAI.Transcription.FastAPI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_OpenAITranscriptionIntegrationTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUTranscriptionIntegration")))
    { AddInfo(TEXT("Skipped; run TestServer/run_transcription_tests.py to start the fixture.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::OpenAITranscriptionTest::FIntegration(this));
    return true;
}
#endif
