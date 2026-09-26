// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_AudioTestActor.h"
#include "JWNU_MicrophoneCaptureComponent.h"
#include "JWNU_PCMPlayerComponent.h"
#include "JWNU_AudioPCM.h"
#include "JWNU_AudioTestWidget.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AJWNU_AudioTestActor::AJWNU_AudioTestActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Microphone = CreateDefaultSubobject<UJWNU_MicrophoneCaptureComponent>(TEXT("Microphone"));
    Player = CreateDefaultSubobject<UJWNU_PCMPlayerComponent>(TEXT("Player"));
    Microphone->OnPCMNative.AddUObject(this, &AJWNU_AudioTestActor::ReceivePCM);
    Microphone->OnErrorNative.AddUObject(this, &AJWNU_AudioTestActor::AudioError);
    Player->OnErrorNative.AddUObject(this, &AJWNU_AudioTestActor::AudioError);
}
bool AJWNU_AudioTestActor::StartRecording()
{
    check(IsInGameThread());
    if (bEnding || bDispatchingError || State != EJWNU_AudioTestState::Idle) { return false; }
    if (!JWNU::AudioIO::IsSupportedSampleRate(RecordingSampleRate)
        || !FMath::IsFinite(MaxRecordingSeconds) || MaxRecordingSeconds < .1f || MaxRecordingSeconds > 10.f)
    { Report(TEXT("지원 표본율과 0.1~10초 녹음 길이를 선택하세요.")); return false; }
    LastError.Reset();
    if (!Microphone->StartCapture(RecordingSampleRate, MicrophoneDeviceIndex)) { return false; }
    RecordedRate = RecordingSampleRate;
    RecordingLimitBytes = FMath::FloorToInt(MaxRecordingSeconds * RecordedRate) * 2;
    Recording.Reset();
    Recording.Reserve(RecordingLimitBytes);
    PlaybackOffset = 0;
    StoppedPlaybackSeconds = 0;
    State = EJWNU_AudioTestState::Recording;
    return true;
}
void AJWNU_AudioTestActor::ReceivePCM(const TArray<uint8>& Bytes)
{
    if (State != EJWNU_AudioTestState::Recording) { return; }
    const int32 Count = FMath::Min(Bytes.Num(), RecordingLimitBytes - Recording.Num());
    Recording.Append(Bytes.GetData(), Count - Count % 2);
    if (Recording.Num() >= RecordingLimitBytes) { StopRecording(); }
}
void AJWNU_AudioTestActor::StopRecording()
{
    check(IsInGameThread());
    if (State != EJWNU_AudioTestState::Recording) { return; }
    State = EJWNU_AudioTestState::Idle;
    Microphone->StopCapture();
}
bool AJWNU_AudioTestActor::LoadRecordingPCM(const TArray<uint8>& PCM16, int32 SampleRate)
{
    check(IsInGameThread());
    if (bEnding || bDispatchingError || State != EJWNU_AudioTestState::Idle || !JWNU::AudioIO::IsSupportedSampleRate(SampleRate)
        || PCM16.IsEmpty() || PCM16.Num() % 2 || PCM16.Num() > SampleRate * 2 * 10) { return false; }
    Recording = PCM16;
    RecordedRate = SampleRate; PlaybackOffset = 0; StoppedPlaybackSeconds = 0; LastError.Reset();
    return true;
}
bool AJWNU_AudioTestActor::PlayRecording()
{
    check(IsInGameThread());
    if (bEnding || bDispatchingError || State != EJWNU_AudioTestState::Idle) { return false; }
    if (Recording.IsEmpty()) { Report(TEXT("먼저 녹음하세요.")); return false; }
    LastError.Reset();
    if (!Player->StartPlayer(RecordedRate)) { Report(TEXT("재생기를 초기화할 수 없습니다.")); return false; }
    PlaybackOffset = 0;
    State = EJWNU_AudioTestState::Playing;
    StoppedPlaybackSeconds = 0;
    LastPlaybackProgress = FPlatformTime::Seconds();
    LastQueuedSeconds = 0;
    SetActorTickEnabled(true);
    FeedPlayback();
    return State == EJWNU_AudioTestState::Playing;
}
void AJWNU_AudioTestActor::FeedPlayback()
{
    // 소비량을 기준으로 최대 약 200ms만 미리 공급한다. Tick 지연 후에도 큐를 폭주시킬 수 없다.
    while (State == EJWNU_AudioTestState::Playing && PlaybackOffset < Recording.Num() && Player->GetBufferedSeconds() < .2f)
    {
        const int32 Count = FMath::Min(RecordedRate / 50 * 2, Recording.Num() - PlaybackOffset);
        TArray<uint8> Chunk;
        Chunk.Append(Recording.GetData() + PlaybackOffset, Count);
        if (!Player->QueuePCM(Chunk))
        {
            if (State == EJWNU_AudioTestState::Playing) { Report(TEXT("PCM 재생 큐가 청크를 거절했습니다.")); }
            return;
        }
        PlaybackOffset += Count;
    }
}
void AJWNU_AudioTestActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (State != EJWNU_AudioTestState::Playing) { return; }
    const float Queued = Player->GetBufferedSeconds();
    if (Queued < LastQueuedSeconds) { LastPlaybackProgress = FPlatformTime::Seconds(); }
    if (PlaybackOffset == Recording.Num() && Queued <= 0)
    {
        StopPlayback();
        return;
    }
    if (FPlatformTime::Seconds() - LastPlaybackProgress > 5.)
    {
        Report(TEXT("5초간 재생 큐가 소비되지 않았습니다. 오디오 출력 장치를 확인하세요."));
        return;
    }
    FeedPlayback();
    LastQueuedSeconds = Player->GetBufferedSeconds();
}
void AJWNU_AudioTestActor::StopPlayback()
{
    check(IsInGameThread());
    if (State != EJWNU_AudioTestState::Playing) { return; }
    StoppedPlaybackSeconds = GetPlaybackSeconds();
    State = EJWNU_AudioTestState::Idle;
    SetActorTickEnabled(false);
    Player->StopPlayer();
}
float AJWNU_AudioTestActor::GetRecordedSeconds() const { return static_cast<float>(Recording.Num()) / (RecordedRate * 2); }
float AJWNU_AudioTestActor::GetPlaybackSeconds() const
{
    if (State != EJWNU_AudioTestState::Playing) { return StoppedPlaybackSeconds; }
    return FMath::Clamp(static_cast<float>(PlaybackOffset) / (RecordedRate * 2) - Player->GetBufferedSeconds(), 0.f, GetRecordedSeconds());
}
float AJWNU_AudioTestActor::GetInputLevel() const { return Microphone->GetInputLevel(); }
void AJWNU_AudioTestActor::Report(const FString& Message)
{
    FJWNU_AudioError Error; Error.Code = TEXT("audio_test"); Error.Message = Message; Error.bFatal = true;
    AudioError(Error);
}
void AJWNU_AudioTestActor::AudioError(const FJWNU_AudioError& Error)
{
    TGuardValue<bool> Guard(bDispatchingError, true);
    StopRecording(); StopPlayback();
    LastError = Error.Message;
    OnError.Broadcast(Error);
}
UJWNU_AudioTestWidget* AJWNU_AudioTestActor::ShowTestPanel(APlayerController* PlayerController)
{
    if (bEnding || !PlayerController || !PlayerController->IsLocalController() || PlayerController->GetWorld() != GetWorld()) { return nullptr; }
    if (!TestPanel)
    {
        TestPanel = CreateWidget<UJWNU_AudioTestWidget>(PlayerController, UJWNU_AudioTestWidget::StaticClass());
        if (!TestPanel) { return nullptr; }
        TestPanel->TestActor = this;
    }
    if (!TestPanel->IsInViewport()) { TestPanel->AddToViewport(); }
    return TestPanel;
}
void AJWNU_AudioTestActor::HideTestPanel()
{
    if (TestPanel) { TestPanel->RemoveFromParent(); TestPanel = nullptr; }
}
void AJWNU_AudioTestActor::EndPlay(const EEndPlayReason::Type Reason)
{
    bEnding = true;
    StopRecording(); StopPlayback(); HideTestPanel();
    Super::EndPlay(Reason);
}
