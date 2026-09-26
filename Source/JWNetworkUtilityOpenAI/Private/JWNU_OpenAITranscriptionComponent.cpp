// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_OpenAITranscriptionComponent.h"
#include "JWNU_OpenAITranscriptionSession.h"
#include "JWNU_MicrophoneCaptureComponent.h"
#include "GameFramework/Actor.h"

bool UJWNU_OpenAITranscriptionComponent::Prepare()
{
    check(IsInGameThread());
    if (bAwaitingClose || bInCallback || !GetOwner() || !IsRegistered()) { return false; }
    Session = UJWNU_OpenAITranscriptionSession::CreateOpenAITranscriptionSession(this);
    if (!Session) { return false; }
    bCaptureThisSession = bUseMicrophone;
    if (bCaptureThisSession && !Microphone)
    {
        Microphone = NewObject<UJWNU_MicrophoneCaptureComponent>(GetOwner());
        Microphone->RegisterComponent();
        Microphone->OnPCMNative.AddUObject(this, &UJWNU_OpenAITranscriptionComponent::Captured);
        Microphone->OnErrorNative.AddUObject(this, &UJWNU_OpenAITranscriptionComponent::AudioError);
    }
    Session->OnReadyNative.AddUObject(this, &UJWNU_OpenAITranscriptionComponent::Ready);
    Session->OnErrorNative.AddUObject(this, &UJWNU_OpenAITranscriptionComponent::Error);
    Session->OnClosedNative.AddUObject(this, &UJWNU_OpenAITranscriptionComponent::Closed);
    Session->OnTranscriptNative.AddWeakLambda(this, [this](const FJWNU_OpenAITranscript& Value)
    { TGuardValue<bool> Guard(bInCallback, true); OnTranscript.Broadcast(Value); });
    Session->OnCommittedNative.AddWeakLambda(this, [this](const FJWNU_OpenAITranscriptionCommit& Value)
    { TGuardValue<bool> Guard(bInCallback, true); OnCommitted.Broadcast(Value); });
    Session->OnRawEventNative.AddWeakLambda(this, [this](const FString& Type, const FString& Json)
    { TGuardValue<bool> Guard(bInCallback, true); OnRawEvent.Broadcast(Type, Json); });
    bAwaitingClose = true;
    return true;
}

bool UJWNU_OpenAITranscriptionComponent::Start(const FJWNU_OpenAITranscriptionOptions& Options, const FString& ApiKey)
{
    if (!Prepare()) { return false; }
    TGuardValue<bool> Guard(bInCallback, true);
    return Session->Start(Options, ApiKey);
}

bool UJWNU_OpenAITranscriptionComponent::StartFromEnvironment(const FJWNU_OpenAITranscriptionOptions& Options)
{
    if (!Prepare()) { return false; }
    TGuardValue<bool> Guard(bInCallback, true);
    return Session->StartFromEnvironment(Options);
}

void UJWNU_OpenAITranscriptionComponent::Ready(const FString& Id)
{
    TGuardValue<bool> Guard(bInCallback, true);
    if (bCaptureThisSession && (!Microphone || !Microphone->StartCapture(24000, MicrophoneDeviceIndex))) { return; }
    if (Session->GetState() == EJWNU_OpenAITranscriptionState::Ready) { OnReady.Broadcast(Id); }
}

void UJWNU_OpenAITranscriptionComponent::Captured(const TArray<uint8>& Bytes)
{
    if (!Session || Session->GetState() != EJWNU_OpenAITranscriptionState::Ready) { return; }
    if (!Session->AppendInputAudio(Bytes) && Session->IsActive())
    {
        FJWNU_OpenAITranscriptionError Info;
        Info.Code = TEXT("audio_limit"); Info.Message = TEXT("Audio was rejected. Commit each turn within 60 seconds."); Info.bFatal = true;
        Error(Info);
    }
}

void UJWNU_OpenAITranscriptionComponent::Error(const FJWNU_OpenAITranscriptionError& Info)
{
    TGuardValue<bool> Guard(bInCallback, true);
    if (Info.bFatal) { StopAudio(); }
    OnError.Broadcast(Info);
    if (Info.bFatal && Session && Session->IsActive()) { Session->Cancel(); }
}

void UJWNU_OpenAITranscriptionComponent::AudioError(const FJWNU_AudioError& Info)
{
    FJWNU_OpenAITranscriptionError Converted;
    Converted.Code = Info.Code; Converted.Message = Info.Message; Converted.bFatal = Info.bFatal;
    Error(Converted);
}

void UJWNU_OpenAITranscriptionComponent::Closed(const FJWNU_OpenAITranscriptionClose& Info)
{
    TGuardValue<bool> Guard(bInCallback, true);
    StopAudio(); bAwaitingClose = false;
    OnClosed.Broadcast(Info);
}

void UJWNU_OpenAITranscriptionComponent::StopAudio() { if (Microphone) { Microphone->StopCapture(); } }
bool UJWNU_OpenAITranscriptionComponent::IsSessionActive() const { return Session && Session->IsActive(); }
bool UJWNU_OpenAITranscriptionComponent::CommitInputAudio()
{
    check(IsInGameThread());
    return Session && Session->CommitInputAudio();
}
void UJWNU_OpenAITranscriptionComponent::Close()
{
    check(IsInGameThread());
    StopAudio();
    if (Session) { Session->Close(); }
}
void UJWNU_OpenAITranscriptionComponent::Cancel()
{
    check(IsInGameThread());
    StopAudio();
    if (Session) { Session->Cancel(); }
}
void UJWNU_OpenAITranscriptionComponent::Cleanup()
{
    bInCallback = true;
    Cancel();
    if (Microphone) { Microphone->DestroyComponent(); Microphone = nullptr; }
}
void UJWNU_OpenAITranscriptionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Cleanup(); Super::EndPlay(Reason);
}
void UJWNU_OpenAITranscriptionComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Cleanup(); Super::OnComponentDestroyed(bDestroyingHierarchy);
}
