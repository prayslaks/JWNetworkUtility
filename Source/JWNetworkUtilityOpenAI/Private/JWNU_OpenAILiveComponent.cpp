// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_OpenAILiveComponent.h"
#include "JWNU_OpenAILiveSession.h"
#include "JWNU_MicrophoneCaptureComponent.h"
#include "JWNU_PCMPlayerComponent.h"
#include "GameFramework/Actor.h"

bool UJWNU_OpenAILiveComponent::Prepare()
{
    check(IsInGameThread());
    if (bAwaitingClose || bInCallback || !GetOwner() || !IsRegistered()) { return false; }
    Session = UJWNU_OpenAILiveSession::CreateOpenAILiveSession(this);
    if (!Session) { return false; }
    bCaptureThisSession = bUseMicrophone;
    bPlayThisSession = bPlayAudio;
    if (!Microphone)
    {
        Microphone = NewObject<UJWNU_MicrophoneCaptureComponent>(GetOwner());
        Microphone->RegisterComponent();
        Microphone->OnPCMNative.AddUObject(this, &UJWNU_OpenAILiveComponent::Captured);
        Microphone->OnErrorNative.AddUObject(this, &UJWNU_OpenAILiveComponent::AudioError);
    }
    if (!Player)
    {
        Player = NewObject<UJWNU_PCMPlayerComponent>(GetOwner());
        Player->RegisterComponent();
        Player->OnErrorNative.AddUObject(this, &UJWNU_OpenAILiveComponent::AudioError);
    }
    Session->OnReadyNative.AddUObject(this, &UJWNU_OpenAILiveComponent::Ready);
    Session->OnAudioNative.AddUObject(this, &UJWNU_OpenAILiveComponent::Audio);
    Session->OnErrorNative.AddUObject(this, &UJWNU_OpenAILiveComponent::Error);
    Session->OnClosedNative.AddUObject(this, &UJWNU_OpenAILiveComponent::Closed);
    Session->OnTranscriptNative.AddWeakLambda(this, [this](const FJWNU_OpenAILiveTranscript& Value) { OnTranscript.Broadcast(Value); });
    Session->OnUsageNative.AddWeakLambda(this, [this](double Seconds) { OnUsage.Broadcast(Seconds); });
    Session->OnRawEventNative.AddWeakLambda(this, [this](const FString& Type, const FString& Json) { OnRawEvent.Broadcast(Type, Json); });
    bAwaitingClose = true;
    return true;
}
bool UJWNU_OpenAILiveComponent::Start(const FJWNU_OpenAILiveOptions& Options, const FString& ApiKey)
{
    if (!Prepare()) { return false; }
    TGuardValue<bool> Guard(bInCallback, true);
    return Session->Start(Options, ApiKey);
}
bool UJWNU_OpenAILiveComponent::StartFromEnvironment(const FJWNU_OpenAILiveOptions& Options)
{
    if (!Prepare()) { return false; }
    TGuardValue<bool> Guard(bInCallback, true);
    return Session->StartFromEnvironment(Options);
}
void UJWNU_OpenAILiveComponent::Ready(const FString& Id)
{
    TGuardValue<bool> Guard(bInCallback, true);
    if (bPlayThisSession && !Player->StartPlayer(Session->GetSampleRate()))
    {
        FJWNU_OpenAILiveError Info; Info.Code = TEXT("playback"); Info.Message = TEXT("Cannot initialize audio playback."); Info.bFatal = true;
        Error(Info); return;
    }
    if (bCaptureThisSession && !Microphone->StartCapture(Session->GetSampleRate(), MicrophoneDeviceIndex)) { return; }
    if (Session->GetState() == EJWNU_OpenAILiveState::Ready) { OnReady.Broadcast(Id); }
}
void UJWNU_OpenAILiveComponent::Audio(const TArray<uint8>& Bytes)
{
    // Close 이후 최종 이벤트를 기다리는 동안 도착하는 PCM은 재생하지 않는다.
    if (bPlayThisSession && Player && Session->GetState() == EJWNU_OpenAILiveState::Ready) { Player->QueuePCM(Bytes); }
    OnAudio.Broadcast(Bytes);
}
void UJWNU_OpenAILiveComponent::Captured(const TArray<uint8>& Bytes)
{
    if (!Session || Session->GetState() != EJWNU_OpenAILiveState::Ready) { return; }
    if (!Session->AppendInputAudio(Bytes))
    {
        FJWNU_OpenAILiveError Info;
        Info.Code = TEXT("backpressure"); Info.Message = TEXT("Audio send queue rejected a microphone chunk."); Info.bFatal = true;
        Error(Info);
    }
}
void UJWNU_OpenAILiveComponent::Error(const FJWNU_OpenAILiveError& Info)
{
    TGuardValue<bool> Guard(bInCallback, true);
    if (Info.bFatal) { StopAudio(); }
    OnError.Broadcast(Info);
    if (Info.bFatal && Session && Session->IsActive()) { Session->Close(); }
}
void UJWNU_OpenAILiveComponent::AudioError(const FJWNU_AudioError& Info)
{
    FJWNU_OpenAILiveError LiveError;
    LiveError.Code = Info.Code; LiveError.Message = Info.Message; LiveError.bFatal = Info.bFatal;
    Error(LiveError);
}
void UJWNU_OpenAILiveComponent::Closed(const FJWNU_OpenAILiveClose& Info)
{
    TGuardValue<bool> Guard(bInCallback, true);
    StopAudio();
    bAwaitingClose = false;
    OnClosed.Broadcast(Info);
}
void UJWNU_OpenAILiveComponent::StopAudio()
{
    if (Microphone) { Microphone->StopCapture(); }
    if (Player) { Player->StopPlayer(); }
}
void UJWNU_OpenAILiveComponent::Close()
{
    check(IsInGameThread());
    StopAudio();
    if (Session) { Session->Close(); }
}
void UJWNU_OpenAILiveComponent::Cancel()
{
    check(IsInGameThread());
    StopAudio();
    if (Session) { Session->Cancel(); }
}
void UJWNU_OpenAILiveComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    bInCallback = true;
    StopAudio();
    if (Session) { Session->Cancel(); }
    if (Microphone) { Microphone->DestroyComponent(); Microphone = nullptr; }
    if (Player) { Player->DestroyComponent(); Player = nullptr; }
    Super::EndPlay(Reason);
}

void UJWNU_OpenAILiveComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    // BeginPlay 전 명시적으로 시작했다가 컴포넌트를 제거해도 세션·장치를 남기지 않는다.
    bInCallback = true;
    StopAudio();
    if (Session) { Session->Cancel(); }
    if (Microphone) { Microphone->DestroyComponent(); Microphone = nullptr; }
    if (Player) { Player->DestroyComponent(); Player = nullptr; }
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}
