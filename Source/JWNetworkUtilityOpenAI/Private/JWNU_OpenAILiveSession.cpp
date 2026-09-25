// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_OpenAILiveSession.h"
#include "JWNU_OpenAILiveJson.h"
#include "JWNU_GIS_OpenAI.h"
#include "JWNU_GIS_WebSocketClient.h"
#include "JWNU_WebSocketConnection.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Base64.h"
#include "UObject/StrongObjectPtr.h"

UJWNU_OpenAILiveSession* UJWNU_OpenAILiveSession::CreateLiveSession(const UObject* Context)
{
    check(IsInGameThread());
    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || World->bIsTearingDown || !World->GetGameInstance()) { return nullptr; }
    auto* Client = World->GetGameInstance()->GetSubsystem<UJWNU_GIS_OpenAI>();
    if (!Client || Client->bStopping) { return nullptr; }
    auto* Session = NewObject<UJWNU_OpenAILiveSession>(Client);
    Session->OwnerClient = Client;
    Session->OwnerWorld = World;
    return Session;
}

bool UJWNU_OpenAILiveSession::IsActive() const
{
    return State == EJWNU_OpenAILiveState::Connecting || State == EJWNU_OpenAILiveState::Starting
        || State == EJWNU_OpenAILiveState::Ready || State == EJWNU_OpenAILiveState::Closing;
}

bool UJWNU_OpenAILiveSession::StartFromEnvironment(const FJWNU_OpenAILiveOptions& Options)
{
    check(IsInGameThread());
    if (State != EJWNU_OpenAILiveState::Idle) { return false; }
    if (Options.Endpoint != TEXT("wss://api.openai.com/v1/live/sessions"))
    {
        Fail(TEXT("configuration"), TEXT("Environment credentials require the official OpenAI endpoint."));
        return false;
    }
    const FString Key = FPlatformMisc::GetEnvironmentVariable(TEXT("OPENAI_API_KEY"));
    if (Key.IsEmpty()) { Fail(TEXT("credentials"), TEXT("OPENAI_API_KEY is not set in this process.")); return false; }
    return Start(Options, Key);
}

bool UJWNU_OpenAILiveSession::Start(const FJWNU_OpenAILiveOptions& Options, const FString& ApiKey)
{
    check(IsInGameThread());
    TStrongObjectPtr<UJWNU_OpenAILiveSession> KeepAlive(this);
    if (State != EJWNU_OpenAILiveState::Idle) { return false; }
    // 비암호화 주소는 키 없는 로컬 모의 서버만 허용한다.
    const bool bLocal = Options.Endpoint.StartsWith(TEXT("ws://127.0.0.1:")) || Options.Endpoint.StartsWith(TEXT("ws://localhost:"));
    if ((!Options.Endpoint.StartsWith(TEXT("wss://")) && !(bLocal && ApiKey.IsEmpty()))
        || (Options.SampleRate != 16000 && Options.SampleRate != 24000)
        || Options.Model.IsEmpty() || Options.BackendModel.IsEmpty() || Options.Voice.IsEmpty()
        || !FMath::IsFinite(Options.StartTimeoutSeconds) || Options.StartTimeoutSeconds <= 0
        || !FMath::IsFinite(Options.CloseTimeoutSeconds) || Options.CloseTimeoutSeconds <= 0)
    {
        Fail(TEXT("configuration"), TEXT("Invalid endpoint, PCM rate, model, voice or timeout."));
        return false;
    }
    if (!bLocal && ApiKey.IsEmpty()) { Fail(TEXT("credentials"), TEXT("A bearer credential is required.")); return false; }
    if (!OwnerClient.IsValid() || !OwnerClient->Track(this))
    {
        Fail(TEXT("world"), TEXT("The owning world is unavailable."));
        return false;
    }
    Settings = Options;
    Socket = UJWNU_GIS_WebSocketClient::CreateConnection(OwnerWorld.Get());
    if (!Socket) { Fail(TEXT("world"), TEXT("Cannot create a WebSocket in this world.")); return false; }
    Socket->OnConnectedNative.AddUObject(this, &UJWNU_OpenAILiveSession::Connected);
    Socket->OnTextMessageNative.AddUObject(this, &UJWNU_OpenAILiveSession::Receive);
    Socket->OnErrorNative.AddUObject(this, &UJWNU_OpenAILiveSession::SocketError);
    Socket->OnClosedNative.AddUObject(this, &UJWNU_OpenAILiveSession::SocketClosed);
    FJWNU_WebSocketOptions Transport;
    Transport.ConnectTimeoutSeconds = Options.StartTimeoutSeconds;
    // 실시간 입력의 적체를 제한한다. 실패 시 호출자가 전송을 중단한다.
    Transport.MaxSendQueuedBytes = 128 * 1024;
    if (!ApiKey.IsEmpty()) { Transport.Headers.Add(TEXT("Authorization"), TEXT("Bearer ") + ApiKey); }
    if (!Options.SafetyIdentifier.IsEmpty()) { Transport.Headers.Add(TEXT("OpenAI-Safety-Identifier"), Options.SafetyIdentifier); }
    State = EJWNU_OpenAILiveState::Connecting;
    Deadline = FPlatformTime::Seconds() + Settings.StartTimeoutSeconds;
    if (!Socket->Connect(Options.Endpoint, Transport))
    {
        if (IsActive()) { Fail(TEXT("connect"), TEXT("WebSocket rejected the connection request.")); }
        return false;
    }
    return true;
}

void UJWNU_OpenAILiveSession::Connected()
{
    if (State != EJWNU_OpenAILiveState::Connecting) { return; }
    State = EJWNU_OpenAILiveState::Starting;
    if (!Send(JWNU::OpenAILive::StartEvent(Settings))) { Fail(TEXT("send"), TEXT("Failed to enqueue session.start.")); }
}

bool UJWNU_OpenAILiveSession::Send(const TSharedRef<FJsonObject>& Event)
{
    return Socket && Socket->SendText(JWNU::OpenAILive::Encode(Event));
}

bool UJWNU_OpenAILiveSession::AppendInputAudio(const TArray<uint8>& PCM16)
{
    check(IsInGameThread());
    if (State != EJWNU_OpenAILiveState::Ready || PCM16.IsEmpty() || PCM16.Num() % 2 || PCM16.Num() > Settings.SampleRate / 5) { return false; }
    auto Event = JWNU::OpenAILive::Event(TEXT("session.input_audio.append"));
    Event->SetStringField(TEXT("audio"), FBase64::Encode(PCM16));
    return Send(Event);
}

bool UJWNU_OpenAILiveSession::SetInputMuted(bool bMuted)
{
    check(IsInGameThread());
    return State == EJWNU_OpenAILiveState::Ready
        && Send(JWNU::OpenAILive::Event(bMuted ? TEXT("session.input_audio.mute") : TEXT("session.input_audio.unmute")));
}

bool UJWNU_OpenAILiveSession::AppendInstructions(const FString& Content)
{
    check(IsInGameThread());
    if (State != EJWNU_OpenAILiveState::Ready || Content.IsEmpty()) { return false; }
    auto Event = JWNU::OpenAILive::Event(TEXT("session.instructions.append"));
    Event->SetStringField(TEXT("content"), Content);
    Event->SetField(TEXT("delegation_id"), MakeShared<FJsonValueNull>());
    return Send(Event);
}

bool UJWNU_OpenAILiveSession::SendEventJson(const FString& Json)
{
    check(IsInGameThread());
    if (State != EJWNU_OpenAILiveState::Ready || Json.Len() > 65536) { return false; }
    TSharedPtr<FJsonObject> Object;
    if (!JWNU::OpenAILive::Decode(Json, Object)) { return false; }
    const FString Type = JWNU::OpenAILive::String(Object, TEXT("type"));
    if (Type.IsEmpty() || Type == TEXT("session.start") || Type == TEXT("session.close") || Type == TEXT("session.input_audio.append")) { return false; }
    if (!Object->HasField(TEXT("event_id"))) { Object->SetStringField(TEXT("event_id"), FGuid::NewGuid().ToString(EGuidFormats::Digits)); }
    return Send(Object.ToSharedRef());
}

bool UJWNU_OpenAILiveSession::Close()
{
    check(IsInGameThread());
    if (State == EJWNU_OpenAILiveState::Connecting || State == EJWNU_OpenAILiveState::Starting) { Abort(); return true; }
    if (State != EJWNU_OpenAILiveState::Ready) { return false; }
    State = EJWNU_OpenAILiveState::Closing;
    Deadline = FPlatformTime::Seconds() + Settings.CloseTimeoutSeconds;
    if (!Send(JWNU::OpenAILive::Event(TEXT("session.close")))) { Fail(TEXT("send"), TEXT("Failed to enqueue session.close.")); return false; }
    return true;
}

void UJWNU_OpenAILiveSession::Abort()
{
    check(IsInGameThread());
    if (!IsActive()) { return; }
    CloseInfo.Reason = TEXT("aborted");
    Finish(false);
}

void UJWNU_OpenAILiveSession::Pump()
{
    if ((State == EJWNU_OpenAILiveState::Connecting || State == EJWNU_OpenAILiveState::Starting || State == EJWNU_OpenAILiveState::Closing)
        && FPlatformTime::Seconds() >= Deadline)
    {
        Fail(TEXT("timeout"), State == EJWNU_OpenAILiveState::Closing
            ? TEXT("session.closed was not received before the close timeout.")
            : TEXT("session.started was not received before the start timeout."));
    }
}

void UJWNU_OpenAILiveSession::Receive(const FString& Json)
{
    TStrongObjectPtr<UJWNU_OpenAILiveSession> KeepAlive(this);
    if (!IsActive()) { return; }
    TSharedPtr<FJsonObject> Object;
    if (!JWNU::OpenAILive::Decode(Json, Object)) { Fail(TEXT("protocol"), TEXT("Expected a JSON object.")); return; }
    const FString Type = JWNU::OpenAILive::String(Object, TEXT("type"));
    if (Type.IsEmpty()) { Fail(TEXT("protocol"), TEXT("Missing event type.")); return; }
    if (Type == TEXT("session.started"))
    {
        const TSharedPtr<FJsonObject>* Session = nullptr;
        if (State != EJWNU_OpenAILiveState::Starting || !Object->TryGetObjectField(TEXT("session"), Session)
            || JWNU::OpenAILive::String(*Session, TEXT("id")).IsEmpty())
        { Fail(TEXT("protocol"), TEXT("Unexpected or malformed session.started.")); return; }
        const TSharedPtr<FJsonObject>* Audio = nullptr;
        const TSharedPtr<FJsonObject>* Format = nullptr;
        double Rate = 0;
        if (!(*Session)->TryGetObjectField(TEXT("audio"), Audio)
            || !(*Audio)->TryGetObjectField(TEXT("format"), Format)
            || JWNU::OpenAILive::String(*Format, TEXT("type")) != TEXT("audio/pcm")
            || !(*Format)->TryGetNumberField(TEXT("rate"), Rate) || Rate != Settings.SampleRate)
        { Fail(TEXT("protocol"), TEXT("Resolved session audio does not match the requested PCM format.")); return; }
        State = EJWNU_OpenAILiveState::Ready;
        const FString Id = JWNU::OpenAILive::String(*Session, TEXT("id"));
        OnReadyNative.Broadcast(Id);
        if (State == EJWNU_OpenAILiveState::Ready) { OnReady.Broadcast(Id); }
    }
    else if (Type == TEXT("session.output_audio.delta"))
    {
        if (State != EJWNU_OpenAILiveState::Ready && State != EJWNU_OpenAILiveState::Closing)
        { Fail(TEXT("protocol"), TEXT("Audio arrived before session.started.")); return; }
        TArray<uint8> Bytes;
        if (!FBase64::Decode(JWNU::OpenAILive::String(Object, TEXT("delta")), Bytes) || Bytes.IsEmpty() || Bytes.Num() % 2)
        { Fail(TEXT("protocol"), TEXT("Invalid PCM16 audio delta.")); return; }
        OnAudioNative.Broadcast(Bytes);
        if (IsActive()) { OnAudio.Broadcast(Bytes); }
    }
    else if (Type == TEXT("session.input_transcript.delta") || Type == TEXT("session.output_transcript.delta"))
    {
        FJWNU_OpenAILiveTranscript Transcript;
        Transcript.bInput = Type == TEXT("session.input_transcript.delta");
        if (!Object->TryGetStringField(TEXT("delta"), Transcript.Delta)
            || !Object->TryGetNumberField(TEXT("start_ms"), Transcript.StartMilliseconds)
            || !Object->TryGetNumberField(TEXT("end_ms"), Transcript.EndMilliseconds)
            || !FMath::IsFinite(Transcript.StartMilliseconds) || !FMath::IsFinite(Transcript.EndMilliseconds)
            || Transcript.StartMilliseconds < 0 || Transcript.EndMilliseconds < Transcript.StartMilliseconds)
        { Fail(TEXT("protocol"), TEXT("Invalid transcript delta.")); return; }
        OnTranscriptNative.Broadcast(Transcript);
        if (IsActive()) { OnTranscript.Broadcast(Transcript); }
    }
    else if (Type == TEXT("session.usage.updated") || Type == TEXT("session.closed"))
    {
        const TSharedPtr<FJsonObject>* Usage = nullptr;
        double Seconds = 0;
        if (!Object->TryGetObjectField(TEXT("usage"), Usage) || !(*Usage)->TryGetNumberField(TEXT("seconds"), Seconds)
            || !FMath::IsFinite(Seconds) || Seconds < 0)
        { Fail(TEXT("protocol"), TEXT("Invalid cumulative session usage.")); return; }
        CloseInfo.UsageSeconds = Seconds;
        OnUsageNative.Broadcast(CloseInfo.UsageSeconds);
        if (IsActive()) { OnUsage.Broadcast(CloseInfo.UsageSeconds); }
        if (Type == TEXT("session.closed") && IsActive())
        {
            CloseInfo.bFinalized = true;
            CloseInfo.Reason = JWNU::OpenAILive::String(Object, TEXT("reason"));
            // 먼저 원본 최종 이벤트를 전달하고, 소켓 close로 남은 송신 큐를 해제한다.
            OnRawEventNative.Broadcast(Type, Json);
            OnRawEvent.Broadcast(Type, Json);
            if (IsActive()) { Finish(false); }
            return;
        }
    }
    else if (Type == TEXT("error"))
    {
        const TSharedPtr<FJsonObject>* Detail = nullptr;
        FJWNU_OpenAILiveError Error;
        if (Object->TryGetObjectField(TEXT("error"), Detail))
        {
            Error.Code = JWNU::OpenAILive::String(*Detail, TEXT("code"));
            Error.Message = JWNU::OpenAILive::String(*Detail, TEXT("message"));
            Error.ClientEventId = JWNU::OpenAILive::String(*Detail, TEXT("client_event_id"));
        }
        if (State != EJWNU_OpenAILiveState::Ready)
        {
            OnRawEventNative.Broadcast(Type, Json); OnRawEvent.Broadcast(Type, Json);
            if (IsActive()) { Fail(Error.Code, Error.Message, Error.ClientEventId); }
            return;
        }
        OnErrorNative.Broadcast(Error);
        if (IsActive()) { OnError.Broadcast(Error); }
    }
    if (IsActive()) { OnRawEventNative.Broadcast(Type, Json); OnRawEvent.Broadcast(Type, Json); }
}

void UJWNU_OpenAILiveSession::SocketError(const FJWNU_WebSocketError& Error) { Fail(TEXT("transport"), Error.Message); }
void UJWNU_OpenAILiveSession::SocketClosed(const FJWNU_WebSocketCloseInfo& Info)
{
    if (!IsActive()) { return; }
    if (Info.bWasLocal)
    {
        CloseInfo.Reason = TEXT("world_cleanup");
        Finish(false);
    }
    else { Fail(TEXT("disconnected"), TEXT("WebSocket closed without session.closed.")); }
}
void UJWNU_OpenAILiveSession::Fail(const FString& Code, const FString& Message, const FString& ClientEventId)
{
    if (State == EJWNU_OpenAILiveState::Closed || State == EJWNU_OpenAILiveState::Failed) { return; }
    TStrongObjectPtr<UJWNU_OpenAILiveSession> KeepAlive(this);
    State = EJWNU_OpenAILiveState::Failed;
    CloseInfo.Reason = Code;
    ReleaseSocket();
    FJWNU_OpenAILiveError Error;
    Error.Code = Code; Error.Message = Message; Error.ClientEventId = ClientEventId; Error.bFatal = true;
    OnErrorNative.Broadcast(Error); OnError.Broadcast(Error);
    OnClosedNative.Broadcast(CloseInfo); OnClosed.Broadcast(CloseInfo);
}
void UJWNU_OpenAILiveSession::Finish(bool bFailed)
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_OpenAILiveSession> KeepAlive(this);
    State = bFailed ? EJWNU_OpenAILiveState::Failed : EJWNU_OpenAILiveState::Closed;
    ReleaseSocket();
    OnClosedNative.Broadcast(CloseInfo); OnClosed.Broadcast(CloseInfo);
}
void UJWNU_OpenAILiveSession::ReleaseSocket()
{
    if (!Socket) { return; }
    Socket->OnConnectedNative.RemoveAll(this);
    Socket->OnTextMessageNative.RemoveAll(this);
    Socket->OnErrorNative.RemoveAll(this);
    Socket->OnClosedNative.RemoveAll(this);
    if (Socket->IsActive()) { Socket->Close(); }
    Socket = nullptr;
}
void UJWNU_OpenAILiveSession::BeginDestroy()
{
    ReleaseSocket();
    Super::BeginDestroy();
}
