// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_OpenAILiveTestReceiver.h"
#include "JWNU_OpenAILiveSession.h"
#include "JWNU_OpenAILiveComponent.h"
#include "JWNU_PCMPlayerComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/GarbageCollection.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"

namespace JWNU::OpenAILiveTest
{
inline UBlueprint* BuildReceiver(FAutomationTestBase* Test)
{
    auto* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_OpenAILiveTestReceiver::StaticClass(), GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_LiveAutomation")),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("ReceiveGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
    auto* Event = NewObject<UK2Node_Event>(Graph);
    Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UJWNU_OpenAILiveTestReceiver, Transcript), UJWNU_OpenAILiveTestReceiver::StaticClass());
    Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
    auto* Record = NewObject<UK2Node_CallFunction>(Graph);
    Record->SetFromFunction(UJWNU_OpenAILiveTestReceiver::StaticClass()->FindFunctionByName(TEXT("Record")));
    Graph->AddNode(Record); Record->CreateNewGuid(); Record->AllocateDefaultPins();
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
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
            FParse::Value(FCommandLine::Get(), TEXT("JWNULiveTestURL="), Base);
            Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone();
            World = Instance->GetWorld();
            BP = BuildReceiver(Test); BP->AddToRoot();
            Payload.SetNumZeroed(960);
            Payload[0] = 0x00; Payload[1] = 0x80; Payload[2] = 0xff; Payload[3] = 0x7f;
            auto* AudioActor = World->SpawnActor<AActor>();
            auto* Player = NewObject<UJWNU_PCMPlayerComponent>(AudioActor);
            Player->RegisterComponent();
            Test->TestTrue(TEXT("PCM player initialized without hardware capture"), Player->StartPlayer());
            Test->TestTrue(TEXT("PCM player accepts samples"), Player->QueuePCM(Payload));
            Test->TestFalse(TEXT("PCM player rejects half sample"), Player->QueuePCM({1}));
            TArray<uint8> TooMuch; TooMuch.SetNumZeroed(96002);
            int32 OverflowCount = 0;
            Player->OnErrorNative.AddLambda([&OverflowCount](const FJWNU_AudioError&) { ++OverflowCount; });
            Test->TestFalse(TEXT("PCM player bounds queued output"), Player->QueuePCM(TooMuch));
            Test->TestEqual(TEXT("PCM overflow notified"), OverflowCount, 1);
            Test->TestEqual(TEXT("PCM overflow releases queue"), Player->GetBufferedSeconds(), 0.f);
            Player->OnErrorNative.Clear();
            Player->DestroyComponent(); AudioActor->Destroy();
        }
        if (Stage == 16) { Cleanup(); return true; }
        if (!Receiver) { Start(); }
        const double Now = FPlatformTime::Seconds();
        if (Now - Started > 6)
        {
            Test->AddError(FString::Printf(TEXT("Live stage %d timeout"), Stage));
            Cleanup(); return true;
        }
        if ((Stage == 0 || Stage == 6) && !bSent && Session->GetState() == EJWNU_OpenAILiveState::Ready)
        {
            bSent = true;
            CollectGarbage(RF_NoFlags);
            Test->TestTrue(TEXT("Active session retained through GC"), Session->IsActive());
            Test->TestTrue(TEXT("20ms input accepted"), Session->AppendInputAudio(Payload));
            Test->TestFalse(TEXT("Odd PCM rejected"), Session->AppendInputAudio({1}));
            TArray<uint8> Oversized; Oversized.SetNumZeroed(4802);
            Test->TestFalse(TEXT("Input over 100ms rejected"), Session->AppendInputAudio(Oversized));
            if (Stage == 0)
            {
                Test->TestTrue(TEXT("Mute command"), Session->SetInputMuted(true));
                Test->TestTrue(TEXT("Unmute command"), Session->SetInputMuted(false));
                Test->TestTrue(TEXT("Context command"), Session->AppendInstructions(TEXT("테스트 지침")));
                Test->TestTrue(TEXT("Raw command error fixture"), Session->SendEventJson(TEXT("{\"type\":\"fixture.unknown\"}")));
                Test->TestFalse(TEXT("Raw close bypass rejected"), Session->SendEventJson(TEXT("{\"type\":\"session.close\"}")));
            }
        }
        if ((Stage == 0 || Stage == 6) && Receiver->AudioCount && !bCloseSent
            && (Stage == 6 || (Receiver->ErrorCount == 1 && Muted == 2 && Appended == 1 && Receiver->Transcripts.Num() == 2)))
        {
            bCloseSent = true;
            Test->TestTrue(TEXT("Close accepted"), Session->Close());
            Test->TestFalse(TEXT("No audio after close request"), Session->AppendInputAudio(Payload));
            Test->TestFalse(TEXT("Double close rejected"), Session->Close());
        }
        if (Receiver->ClosedCount == 0) { return false; }
        if (!Finished) { Finished = Now; return false; }
        if (Now - Finished < .1) { return false; }
        Validate();
        if (Actor) { Actor->Destroy(); Actor = nullptr; Component = nullptr; }
        Receiver->RemoveFromRoot(); Receiver = nullptr; Session = nullptr; ++Stage;
        return false;
    }
private:
    void Start()
    {
        Test->AddInfo(FString::Printf(TEXT("Live stage %d"), Stage));
        Started = FPlatformTime::Seconds(); Finished = 0; bSent = false; bCloseSent = false;
        NativeReady = 0; NativeClosed = 0; Muted = 0; Appended = 0;
        Receiver = NewObject<UJWNU_OpenAILiveTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get());
        Receiver->AddToRoot();
        FJWNU_OpenAILiveOptions Options;
        const TCHAR* Modes[] = { TEXT("echo"), TEXT("reject"), TEXT("start_timeout"), TEXT("malformed"), TEXT("bad_audio"), TEXT("drop"), TEXT("close_timeout") };
        Options.Endpoint = Base + TEXT("/live/sessions?mode=") + (Stage < 7 ? Modes[Stage] : TEXT("echo"));
        if (Stage == 13) { Options.Endpoint = Base + TEXT("/live/sessions?mode=bad_format"); }
        if (Stage == 14) { Options.Endpoint = Base + TEXT("/live/sessions?mode=bad_usage"); }
        Options.StartTimeoutSeconds = .8f; Options.CloseTimeoutSeconds = .5f;
        if (Stage == 0 || Stage == 12)
        {
            Actor = World->SpawnActor<AActor>();
            Component = NewObject<UJWNU_OpenAILiveComponent>(Actor);
            Component->bUseMicrophone = false; Component->bPlayAudio = false;
            Component->RegisterComponent();
            Component->OnReady.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Ready);
            Component->OnTranscript.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Transcript);
            Component->OnAudio.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Audio);
            Component->OnError.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Error);
            Component->OnClosed.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Closed);
            Test->TestTrue(TEXT("Component starts"), Component->StartLive(Options, TEXT("")));
            Session = Component->GetSession();
        }
        else
        {
            Session = UJWNU_OpenAILiveSession::CreateLiveSession(World);
            Session->OnReady.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Ready);
            Session->OnTranscript.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Transcript);
            Session->OnAudio.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Audio);
            Session->OnError.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Error);
            Session->OnClosed.AddDynamic(Receiver, &UJWNU_OpenAILiveTestReceiver::Closed);
        }
        Session->OnClosedNative.AddLambda([this](const FJWNU_OpenAILiveClose&) { ++NativeClosed; });
        Session->OnReadyNative.AddLambda([this](const FString&)
        {
            ++NativeReady;
            if (Stage == 8) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); CollectGarbage(RF_NoFlags); }
            if (Stage == 11) { Session->Abort(); CollectGarbage(RF_NoFlags); }
            if (Stage == 12) { Component->DestroyComponent(); }
            if (Stage == 15) { Instance->Shutdown(); bShutdown = true; }
        });
        Session->OnRawEventNative.AddLambda([this](const FString& Type, const FString&)
        {
            if (Type == TEXT("session.input_audio.muted") || Type == TEXT("session.input_audio.unmuted")) { ++Muted; }
            if (Type == TEXT("session.instructions.appended")) { ++Appended; }
        });
        if (Stage == 9)
        {
            Test->TestFalse(TEXT("Environment key cannot go to custom endpoint"), Session->StartFromEnvironment(Options));
        }
        else if (Stage == 10)
        {
            Options.SampleRate = 44100;
            Test->TestFalse(TEXT("Unsupported format rejected"), Session->Start(Options, TEXT("")));
        }
        else if (Stage != 0 && Stage != 12) { Test->TestTrue(TEXT("Start accepted"), Session->Start(Options, TEXT(""))); }
        Test->TestFalse(TEXT("Audio before ready rejected"), Session->AppendInputAudio(Payload));
        if (Stage == 7) { Test->TestTrue(TEXT("Cancel pending startup"), Session->Close()); }
    }
    void Validate()
    {
        const bool bFailure = (Stage >= 1 && Stage <= 6) || Stage == 9 || Stage == 10 || Stage == 13 || Stage == 14;
        Test->TestEqual(TEXT("Closed once BP"), Receiver->ClosedCount, 1);
        Test->TestEqual(TEXT("Closed once native"), NativeClosed, 1);
        Test->TestEqual(TEXT("Error count"), Receiver->ErrorCount, (bFailure || Stage == 0) ? 1 : 0);
        Test->TestFalse(TEXT("Terminal inactive"), Session->IsActive());
        Test->TestFalse(TEXT("Session is single use"), Session->Start({}, TEXT("")));
        if (Stage == 0)
        {
            Test->TestEqual(TEXT("Component ready BP"), Receiver->ReadyCount, 1);
            Test->TestTrue(TEXT("PCM preserved"), Receiver->LastAudio == Payload);
            Test->TestEqual(TEXT("Two speaker transcripts via compiled BP"), Receiver->Transcripts.Num(), 2);
            if (Receiver->Transcripts.Num() == 2)
            {
                Test->TestEqual(TEXT("UTF8 transcript and spaces preserved"), Receiver->Transcripts[0].Delta, FString(TEXT("안녕 🙂 ")));
                Test->TestTrue(TEXT("Input speaker"), Receiver->Transcripts[0].bInput);
                Test->TestFalse(TEXT("Output speaker"), Receiver->Transcripts[1].bInput);
                Test->TestEqual(TEXT("Timeline preserved"), Receiver->Transcripts[0].EndMilliseconds, 20.);
            }
            Test->TestTrue(TEXT("Final usage received"), Receiver->LastClose.bFinalized);
            Test->TestEqual(TEXT("Cumulative usage, not sum"), Receiver->LastClose.UsageSeconds, 2.);
            Test->TestFalse(TEXT("Command error nonfatal"), Receiver->LastError.bFatal);
            Test->TestFalse(TEXT("Command error correlation"), Receiver->LastError.ClientEventId.IsEmpty());
        }
        else { Test->TestFalse(TEXT("Incomplete finalization identified"), Receiver->LastClose.bFinalized); }
        if (bFailure) { Test->TestTrue(TEXT("Fatal error marked"), Receiver->LastError.bFatal); }
        if (Stage == 1) { Test->TestFalse(TEXT("Startup error correlation preserved"), Receiver->LastError.ClientEventId.IsEmpty()); }
        if (Stage == 2 || Stage == 6) { Test->TestEqual(TEXT("Deadline failure"), Receiver->LastError.Code, FString(TEXT("timeout"))); }
    }
    void Cleanup()
    {
        if (Session) { Session->Abort(); }
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
    UJWNU_OpenAILiveComponent* Component = nullptr;
    UJWNU_OpenAILiveSession* Session = nullptr;
    UJWNU_OpenAILiveTestReceiver* Receiver = nullptr;
    FString Base;
    TArray<uint8> Payload;
    int32 Stage = 0, NativeReady = 0, NativeClosed = 0, Muted = 0, Appended = 0;
    double Started = 0, Finished = 0;
    bool bSent = false, bCloseSent = false, bShutdown = false;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_OpenAILiveIntegrationTest, "JWNetworkUtility.OpenAI.Live.FastAPI",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_OpenAILiveIntegrationTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("JWNULiveIntegration")))
    { AddInfo(TEXT("Skipped; run TestServer/run_live_tests.py to start the fixture.")); return true; }
    ADD_LATENT_AUTOMATION_COMMAND(JWNU::OpenAILiveTest::FIntegration(this));
    return true;
}
#endif
