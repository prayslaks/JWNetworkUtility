// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_AudioTestActor.h"
#include "JWNU_AudioTestWidget.h"
#include "JWNU_MicrophoneCaptureComponent.h"
#include "JWNU_PCMPlayerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundWaveProcedural.h"
#include "UObject/CoreRedirects.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_AudioWorkflowTest, "JWNetworkUtility.Audio.UMGWorkflow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_AudioWorkflowTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("nosound")))
    { AddInfo(TEXT("Skipped; run TestServer/run_audio_tests.py for deterministic device-free playback.")); return true; }
    for (const auto& Pair : {
        TPair<FString, FString>(TEXT("/Script/JWNetworkUtilityOpenAI.JWNU_OpenAIMicrophoneComponent"), TEXT("/Script/JWNetworkUtilityAudio.JWNU_MicrophoneCaptureComponent")),
        TPair<FString, FString>(TEXT("/Script/JWNetworkUtilityOpenAI.JWNU_OpenAIAudioPlayerComponent"), TEXT("/Script/JWNetworkUtilityAudio.JWNU_PCMPlayerComponent"))})
    {
        const auto Redirected = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class, FCoreRedirectObjectName(Pair.Key));
        TestEqual(TEXT("Serialized component class redirect"), Redirected.ToString(), Pair.Value);
    }
    auto* Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone();
    UWorld* World = Instance->GetWorld();
    auto* Actor = World->SpawnActor<AJWNU_AudioTestActor>();
    TestNotNull(TEXT("Test actor"), Actor);
    if (!Actor) { Instance->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false); Instance->RemoveFromRoot(); return false; }
    TestFalse(TEXT("No automatic microphone capture"), Actor->Microphone->IsCapturing());
    TArray<uint8> Clip;
    for (int32 Index = 0; Index < 24000 * 5 + 137; ++Index)
    {
        const uint16 Sample = static_cast<uint16>(static_cast<int16>(12000 * FMath::Sin(2. * PI * 440. * Index / 24000.)));
        Clip.Add(static_cast<uint8>(Sample & 0xff)); Clip.Add(static_cast<uint8>(Sample >> 8));
    }
    TestFalse(TEXT("Odd recording rejected"), Actor->LoadRecordingPCM({1}, 24000));
    TestFalse(TEXT("Unsupported rate rejected"), Actor->LoadRecordingPCM({0, 0}, 12345));
    TArray<uint8> Oversized; Oversized.SetNumZeroed(480002);
    TestFalse(TEXT("Recording memory bounded at ten seconds"), Actor->LoadRecordingPCM(Oversized, 24000));
    TestTrue(TEXT("Load >2-second recording"), Actor->LoadRecordingPCM(Clip, 24000));
    auto* Widget = CreateWidget<UJWNU_AudioTestWidget>(Instance, UJWNU_AudioTestWidget::StaticClass());
    TestNotNull(TEXT("Native UMG panel"), Widget);
    if (Widget)
    {
        Widget->AddToRoot(); Widget->TestActor = Actor; Widget->TakeWidget();
        TArray<UButton*> Buttons;
        Widget->WidgetTree->ForEachWidget([&Buttons](UWidget* Child) { if (auto* Button = Cast<UButton>(Child)) { Buttons.Add(Button); } });
        TestEqual(TEXT("Four UMG controls"), Buttons.Num(), 4);
        for (auto* Button : Buttons)
        {
            auto* Label = Cast<UTextBlock>(Button->GetChildAt(0));
            if (Label && Label->GetText().ToString() == TEXT("녹음 재생")) { Button->OnClicked.Broadcast(); }
        }
        TestEqual(TEXT("UMG play click reaches Actor"), Actor->GetTestState(), EJWNU_AudioTestState::Playing);
        TestTrue(TEXT("Initial queue stays short"), Actor->Player->GetBufferedSeconds() <= .221f);
        TestFalse(TEXT("Concurrent play rejected"), Actor->PlayRecording());
        TestFalse(TEXT("Replace active clip rejected"), Actor->LoadRecordingPCM(Clip, 24000));
        auto* WaveProperty = FindFProperty<FObjectPropertyBase>(UJWNU_PCMPlayerComponent::StaticClass(), TEXT("Wave"));
        auto* Wave = WaveProperty ? Cast<USoundWaveProcedural>(WaveProperty->GetObjectPropertyValue_InContainer(Actor->Player)) : nullptr;
        TestNotNull(TEXT("Procedural player wave"), Wave);
        TArray<uint8> Played, Scratch; Scratch.SetNumUninitialized(4096);
        // -nosound 환경에서만 오디오 소비를 결정적으로 모사한다.
        for (int32 Tick = 0; Wave && Tick < 300 && Actor->GetTestState() == EJWNU_AudioTestState::Playing; ++Tick)
        {
            const int32 Count = Wave->GeneratePCMData(Scratch.GetData(), 1024);
            Played.Append(Scratch.GetData(), Count);
            Actor->Tick(.02f);
            TestTrue(TEXT("Paced queue below streaming cap"), Actor->Player->GetBufferedSeconds() <= .221f);
        }
        TestTrue(TEXT("Full recording and final partial chunk preserved"), Played == Clip);
        TestEqual(TEXT("Playback drains to idle"), Actor->GetTestState(), EJWNU_AudioTestState::Idle);
        TestEqual(TEXT("Completed playback duration"), Actor->GetPlaybackSeconds(), Actor->GetRecordedSeconds());
        TestTrue(TEXT("Replay accepted"), Actor->PlayRecording());
        for (auto* Button : Buttons)
        {
            auto* Label = Cast<UTextBlock>(Button->GetChildAt(0));
            if (Label && Label->GetText().ToString() == TEXT("재생 정지")) { Button->OnClicked.Broadcast(); }
        }
        TestEqual(TEXT("UMG stop reaches Actor"), Actor->GetTestState(), EJWNU_AudioTestState::Idle);
        TestEqual(TEXT("Stop does not advance playback through unplayed buffer"), Actor->GetPlaybackSeconds(), 0.f);
        Widget->TestActor = nullptr; Widget->RemoveFromRoot();
    }
    // 장치를 열지 않고 같은 마이크 PCM 이벤트 경로로 녹음 상한·자동 정지를 검증한다.
    Actor->Recording.Reset(); Actor->RecordedRate = 24000;
    Actor->RecordingLimitBytes = 4800; Actor->State = EJWNU_AudioTestState::Recording;
    TArray<uint8> Chunk; Chunk.SetNumZeroed(960);
    for (int32 Index = 0; Index < 7; ++Index) { Actor->Microphone->OnPCMNative.Broadcast(Chunk); }
    TestEqual(TEXT("Recording auto stop"), Actor->GetTestState(), EJWNU_AudioTestState::Idle);
    TestEqual(TEXT("Recording capped exactly"), Actor->Recording.Num(), 4800);
    TestFalse(TEXT("Recording test never acquired hardware"), Actor->Microphone->IsCapturing());
    TestTrue(TEXT("Generic 44.1k player initializes"), Actor->Player->StartPlayer(44100));
    Actor->Player->StopPlayer();
    Actor->Destroy(); Instance->Shutdown();
    GEngine->DestroyWorldContext(World); World->DestroyWorld(false); Instance->RemoveFromRoot();
    return true;
}
#endif
