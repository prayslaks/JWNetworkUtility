// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_OpenAITranscriptionSession.h"
#include "JWNU_OpenAITranscriptionJson.h"
#include "JWNU_GIS_OpenAI.h"
#include "JWNU_GIS_WebSocketClient.h"
#include "JWNU_WebSocketConnection.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Base64.h"
#include "UObject/StrongObjectPtr.h"

UJWNU_OpenAITranscriptionSession* UJWNU_OpenAITranscriptionSession::CreateOpenAITranscriptionSession(const UObject* Context)
{
    check(IsInGameThread());
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || World->bIsTearingDown || !World->GetGameInstance()) { return nullptr; }
    auto* Client = World->GetGameInstance()->GetSubsystem<UJWNU_GIS_OpenAI>();
    if (!Client || Client->bStopping) { return nullptr; }
    auto* Session = NewObject<UJWNU_OpenAITranscriptionSession>(Client);
    Session->OwnerClient = Client;
    Session->OwnerWorld = World;
    return Session;
}

bool UJWNU_OpenAITranscriptionSession::IsActive() const
{
    return State == EJWNU_OpenAITranscriptionState::Connecting || State == EJWNU_OpenAITranscriptionState::Starting
        || State == EJWNU_OpenAITranscriptionState::Ready || State == EJWNU_OpenAITranscriptionState::Closing;
}

TArray<FString> UJWNU_OpenAITranscriptionSession::GetKnownTranscriptionModels()
{
    using namespace JWNU::OpenAITranscription;
    return {Models::LiveTranscribe, Models::Transcribe, Models::RealtimeWhisper};
}

bool UJWNU_OpenAITranscriptionSession::StartFromEnvironment(const FJWNU_OpenAITranscriptionOptions& Options)
{
    check(IsInGameThread());
    if (State != EJWNU_OpenAITranscriptionState::Idle) { return false; }
    if (Options.Endpoint != FJWNU_OpenAITranscriptionOptions{}.Endpoint)
    { Fail(TEXT("configuration"), TEXT("Environment credentials require the official OpenAI transcription endpoint.")); return false; }
    const FString Key = FPlatformMisc::GetEnvironmentVariable(TEXT("OPENAI_API_KEY"));
    if (Key.IsEmpty()) { Fail(TEXT("credentials"), TEXT("OPENAI_API_KEY is not set in this process.")); return false; }
    return Start(Options, Key);
}

bool UJWNU_OpenAITranscriptionSession::Start(const FJWNU_OpenAITranscriptionOptions& Options, const FString& ApiKey)
{
    check(IsInGameThread());
    TStrongObjectPtr<UJWNU_OpenAITranscriptionSession> KeepAlive(this);
    if (State != EJWNU_OpenAITranscriptionState::Idle) { return false; }
    const bool bLocal = Options.Endpoint.StartsWith(TEXT("ws://127.0.0.1:")) || Options.Endpoint.StartsWith(TEXT("ws://localhost:"));
    if ((!Options.Endpoint.StartsWith(TEXT("wss://")) && !(bLocal && ApiKey.IsEmpty()))
        || !JWNU::OpenAITranscription::ValidOptions(Options))
    { Fail(TEXT("configuration"), TEXT("Invalid endpoint, model name, timeout or transcription hint format.")); return false; }
    if (!bLocal && ApiKey.IsEmpty()) { Fail(TEXT("credentials"), TEXT("An API key is required.")); return false; }
    if (!OwnerClient.IsValid() || !OwnerClient->Track(this))
    { Fail(TEXT("world"), TEXT("The owning world is unavailable.")); return false; }
    Settings = Options;
    Socket = UJWNU_GIS_WebSocketClient::CreateConnection(OwnerWorld.Get());
    if (!Socket) { Fail(TEXT("world"), TEXT("Cannot create a WebSocket in this world.")); return false; }
    Socket->OnConnectedNative.AddUObject(this, &UJWNU_OpenAITranscriptionSession::Connected);
    Socket->OnTextMessageNative.AddUObject(this, &UJWNU_OpenAITranscriptionSession::Receive);
    Socket->OnErrorNative.AddUObject(this, &UJWNU_OpenAITranscriptionSession::SocketError);
    Socket->OnClosedNative.AddUObject(this, &UJWNU_OpenAITranscriptionSession::SocketClosed);
    FJWNU_WebSocketOptions Transport;
    Transport.ConnectTimeoutSeconds = Options.StartTimeoutSeconds;
    Transport.MaxSendQueuedBytes = 128 * 1024;
    if (!ApiKey.IsEmpty()) { Transport.Headers.Add(TEXT("Authorization"), TEXT("Bearer ") + ApiKey); }
    State = EJWNU_OpenAITranscriptionState::Connecting;
    Deadline = FPlatformTime::Seconds() + Settings.StartTimeoutSeconds;
    if (!Socket->Connect(Options.Endpoint, Transport))
    {
        if (IsActive()) { Fail(TEXT("connect"), TEXT("WebSocket rejected the connection request.")); }
        return false;
    }
    return true;
}

void UJWNU_OpenAITranscriptionSession::Connected()
{
    if (State != EJWNU_OpenAITranscriptionState::Connecting) { return; }
    State = EJWNU_OpenAITranscriptionState::Starting;
    Send(JWNU::OpenAITranscription::UpdateEvent(Settings));
}

bool UJWNU_OpenAITranscriptionSession::Send(const TSharedRef<FJsonObject>& Event)
{
    if (Socket && Socket->SendText(JWNU::OpenAITranscription::Encode(Event))) { return true; }
    if (IsActive()) { Fail(TEXT("send"), TEXT("The bounded WebSocket send queue rejected an event.")); }
    return false;
}

bool UJWNU_OpenAITranscriptionSession::AppendInputAudio(const TArray<uint8>& PCM16)
{
    check(IsInGameThread());
    if (State != EJWNU_OpenAITranscriptionState::Ready || PCM16.IsEmpty() || PCM16.Num() % 2
        || PCM16.Num() > 4800 || BufferedBytes + PCM16.Num() > 48000 * 60) { return false; }
    auto Event = JWNU::OpenAITranscription::Event(TEXT("input_audio_buffer.append"));
    Event->SetStringField(TEXT("audio"), FBase64::Encode(PCM16));
    if (!Send(Event)) { return false; }
    BufferedBytes += PCM16.Num();
    return true;
}

bool UJWNU_OpenAITranscriptionSession::CommitInputAudio()
{
    check(IsInGameThread());
    if (State != EJWNU_OpenAITranscriptionState::Ready || BufferedBytes < 4800
        || AwaitingCommits + PendingItems.Num() >= 32) { return false; }
    if (!Send(JWNU::OpenAITranscription::Event(TEXT("input_audio_buffer.commit")))) { return false; }
    BufferedBytes = 0;
    ++AwaitingCommits;
    return true;
}

bool UJWNU_OpenAITranscriptionSession::ClearInputAudio()
{
    check(IsInGameThread());
    if (State != EJWNU_OpenAITranscriptionState::Ready) { return false; }
    if (!Send(JWNU::OpenAITranscription::Event(TEXT("input_audio_buffer.clear")))) { return false; }
    BufferedBytes = 0;
    return true;
}

bool UJWNU_OpenAITranscriptionSession::Close()
{
    check(IsInGameThread());
    TStrongObjectPtr<UJWNU_OpenAITranscriptionSession> KeepAlive(this);
    if (State == EJWNU_OpenAITranscriptionState::Connecting || State == EJWNU_OpenAITranscriptionState::Starting) { Cancel(); return true; }
    if (State != EJWNU_OpenAITranscriptionState::Ready) { return false; }
    // Realtime에는 Live의 session.close가 없다. 제출한 모든 Item의 종료를 기다린 뒤 소켓을 닫는다.
    if (BufferedBytes > 0)
    {
        if (BufferedBytes < 4800)
        {
            TArray<uint8> Padding; Padding.SetNumZeroed(4800 - BufferedBytes);
            if (!AppendInputAudio(Padding)) { return false; }
        }
        if (!CommitInputAudio())
        {
            if (IsActive()) { Fail(TEXT("pending_limit"), TEXT("Too many pending transcription turns to close.")); }
            return false;
        }
    }
    State = EJWNU_OpenAITranscriptionState::Closing;
    Deadline = FPlatformTime::Seconds() + Settings.CloseTimeoutSeconds;
    TryFinishClose();
    return true;
}

void UJWNU_OpenAITranscriptionSession::Cancel()
{
    check(IsInGameThread());
    if (IsActive()) { Finish(TEXT("cancelled"), false); }
}

void UJWNU_OpenAITranscriptionSession::TryFinishClose()
{
    if (State == EJWNU_OpenAITranscriptionState::Closing && AwaitingCommits == 0 && PendingItems.IsEmpty())
    { Finish(TEXT("client_request"), !bHadTranscriptionFailure); }
}

void UJWNU_OpenAITranscriptionSession::Pump()
{
    if ((State == EJWNU_OpenAITranscriptionState::Connecting || State == EJWNU_OpenAITranscriptionState::Starting
        || State == EJWNU_OpenAITranscriptionState::Closing) && FPlatformTime::Seconds() >= Deadline)
    {
        Fail(TEXT("timeout"), State == EJWNU_OpenAITranscriptionState::Closing
            ? TEXT("Final transcripts were not received before the close timeout.")
            : TEXT("session.updated was not received before the start timeout."));
    }
}

void UJWNU_OpenAITranscriptionSession::SocketError(const FJWNU_WebSocketError& Error) { Fail(TEXT("transport"), Error.Message); }
void UJWNU_OpenAITranscriptionSession::SocketClosed(const FJWNU_WebSocketCloseInfo& Info)
{
    if (!IsActive()) { return; }
    if (Info.bWasLocal) { Finish(TEXT("world_cleanup"), false); }
    else { Fail(TEXT("disconnected"), TEXT("WebSocket closed before the transcription session finished.")); }
}

void UJWNU_OpenAITranscriptionSession::Fail(const FString& Code, const FString& Message, const FString& EventId)
{
    if (State == EJWNU_OpenAITranscriptionState::Closed || State == EJWNU_OpenAITranscriptionState::Failed) { return; }
    TStrongObjectPtr<UJWNU_OpenAITranscriptionSession> KeepAlive(this);
    State = EJWNU_OpenAITranscriptionState::Failed;
    ReleaseSocket();
    FJWNU_OpenAITranscriptionError Error;
    Error.Code = Code; Error.Message = Message; Error.EventId = EventId; Error.bFatal = true;
    OnErrorNative.Broadcast(Error); OnError.Broadcast(Error);
    FJWNU_OpenAITranscriptionClose Info; Info.Reason = Code;
    OnClosedNative.Broadcast(Info); OnClosed.Broadcast(Info);
}

void UJWNU_OpenAITranscriptionSession::Finish(const FString& Reason, bool bFinalized)
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_OpenAITranscriptionSession> KeepAlive(this);
    State = EJWNU_OpenAITranscriptionState::Closed;
    ReleaseSocket();
    FJWNU_OpenAITranscriptionClose Info; Info.Reason = Reason; Info.bFinalized = bFinalized;
    OnClosedNative.Broadcast(Info); OnClosed.Broadcast(Info);
}

void UJWNU_OpenAITranscriptionSession::ReleaseSocket()
{
    PendingItems.Reset(); AwaitingCommits = 0; BufferedBytes = 0;
    if (!Socket) { return; }
    Socket->OnConnectedNative.RemoveAll(this);
    Socket->OnTextMessageNative.RemoveAll(this);
    Socket->OnErrorNative.RemoveAll(this);
    Socket->OnClosedNative.RemoveAll(this);
    if (Socket->IsActive()) { Socket->Close(); }
    Socket = nullptr;
}

void UJWNU_OpenAITranscriptionSession::BeginDestroy()
{
    ReleaseSocket();
    Super::BeginDestroy();
}
