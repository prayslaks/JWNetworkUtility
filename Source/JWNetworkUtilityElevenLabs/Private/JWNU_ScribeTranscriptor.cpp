// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_ScribeTranscriptor.h"

#include "JWNU_GIS_WebSocketClient.h"
#include "JWNU_WebSocketConnection.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Misc/Base64.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"

UJWNU_ScribeTranscriptor* UJWNU_ScribeTranscriptor::CreateScribeTranscriptor(const UObject* Context)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || World->bIsTearingDown) return nullptr;
	auto* Result = NewObject<UJWNU_ScribeTranscriptor>(World);
	Result->OwnerWorld = World;
	return Result;
}

UWorld* UJWNU_ScribeTranscriptor::GetWorld() const { return OwnerWorld.Get(); }

bool UJWNU_ScribeTranscriptor::Start(const FJWNU_ScribeOptions& Options, const FString& ApiKey)
{
	check(IsInGameThread());
	TStrongObjectPtr<UJWNU_ScribeTranscriptor> KeepAlive(this);
	if (bActive) return false;
	Cancel();
	const bool bLocal = Options.Endpoint.StartsWith(TEXT("ws://127.0.0.1:")) || Options.Endpoint.StartsWith(TEXT("ws://localhost:"));
	if ((!Options.Endpoint.StartsWith(TEXT("wss://")) && !(bLocal && ApiKey.IsEmpty()))
		|| Options.Endpoint.Contains(TEXT("?")) || Options.Endpoint.Contains(TEXT("#")) || Options.Language.Len() > 16
		|| !FMath::IsFinite(Options.StartTimeoutSeconds) || Options.StartTimeoutSeconds < 1
		|| !FMath::IsFinite(Options.FinishDrainSeconds) || Options.FinishDrainSeconds < 2
		|| (!bLocal && ApiKey.IsEmpty()) || ApiKey.Contains(TEXT("\r")) || ApiKey.Contains(TEXT("\n")))
	{ Fail(TEXT("Scribe endpoint, credentials or timeout configuration is invalid.")); return false; }
	if (!GetWorld() || GetWorld()->bIsTearingDown) { Fail(TEXT("Scribe requires an active world.")); return false; }
	Settings = Options;
	Socket = UJWNU_GIS_WebSocketClient::CreateConnection(GetWorld());
	if (!Socket) { Fail(TEXT("Cannot create Scribe WebSocket.")); return false; }
	const uint32 Epoch = Generation;
	Socket->OnTextMessageNative.AddWeakLambda(this, [this, Epoch](const FString& Json) { if (Generation == Epoch) Receive(Json); });
	Socket->OnErrorNative.AddWeakLambda(this, [this, Epoch](const FJWNU_WebSocketError&)
	{ if (Generation == Epoch) Fail(TEXT("Scribe WebSocket failed.")); });
	Socket->OnClosedNative.AddWeakLambda(this, [this, Epoch](const FJWNU_WebSocketCloseInfo&)
	{ if (Generation == Epoch && bActive) Fail(TEXT("Scribe connection closed before local completion.")); });
	// 공급자 사양은 구현에 고정한다. 호스트는 ProviderManaged 정책만 읽는다.
	FString URL = Options.Endpoint + TEXT("?model_id=scribe_v2_realtime&audio_format=pcm_16000&commit_strategy=vad&vad_silence_threshold_secs=0.7&min_speech_duration_ms=100&min_silence_duration_ms=100&include_timestamps=false");
	if (!Options.Language.IsEmpty()) URL += TEXT("&language_code=") + FGenericPlatformHttp::UrlEncode(Options.Language);
	FJWNU_WebSocketOptions Transport;
	Transport.ConnectTimeoutSeconds = Options.StartTimeoutSeconds;
	Transport.MaxSendQueuedBytes = 256 * 1024;
	Transport.MaxMessageBytes = 128 * 1024;
	if (!ApiKey.IsEmpty()) Transport.Headers.Add(TEXT("xi-api-key"), ApiKey);
	bActive = true;
	Deadline = FPlatformTime::Seconds() + Options.StartTimeoutSeconds;
	const bool bConnected = Socket->Connect(URL, Transport);
	if (!bConnected && Generation == Epoch) Fail(TEXT("Scribe connection request was rejected."));
	return bConnected && Generation == Epoch && bActive;
}

bool UJWNU_ScribeTranscriptor::SendChunk(const TArray<float>& Audio, bool bCommit)
{
	TArray<uint8> PCM;
	PCM.SetNumUninitialized(Audio.Num() * 2);
	for (int32 Index = 0; Index < Audio.Num(); ++Index)
	{
		const float Value = FMath::IsFinite(Audio[Index]) ? FMath::Clamp(Audio[Index], -1.f, 1.f) : 0.f;
		const int16 Sample = static_cast<int16>(FMath::RoundToInt(Value * (Value < 0 ? 32768.f : 32767.f)));
		PCM[Index * 2] = static_cast<uint8>(Sample & 255);
		PCM[Index * 2 + 1] = static_cast<uint8>((static_cast<uint16>(Sample) >> 8) & 255);
	}
	auto Event = MakeShared<FJsonObject>();
	Event->SetStringField(TEXT("message_type"), TEXT("input_audio_chunk"));
	Event->SetStringField(TEXT("audio_base_64"), FBase64::Encode(PCM));
	Event->SetNumberField(TEXT("sample_rate"), GetSampleRate());
	Event->SetBoolField(TEXT("commit"), bCommit);
	FString Json;
	FJsonSerializer::Serialize(Event, TJsonWriterFactory<>::Create(&Json));
	if (!Socket || !Socket->SendText(Json)) { Fail(TEXT("Scribe bounded send queue rejected audio.")); return false; }
	SentSamples += Audio.Num();
	return true;
}

void UJWNU_ScribeTranscriptor::AppendAudio(const TArray<float>& Audio)
{
	check(IsInGameThread());
	TStrongObjectPtr<UJWNU_ScribeTranscriptor> KeepAlive(this);
	if (!bReady || bFinishing) return;
	if (Audio.Num() > GetSampleRate() * 10) { Fail(TEXT("Scribe input batch exceeds ten seconds.")); return; }
	// 전체 세션 길이는 누적 제한하지 않고 미전송 tail만 최대 100ms로 보유한다.
	for (int32 Offset = 0; Offset < Audio.Num();)
	{
		const int32 Count = FMath::Min(1600 - PendingAudio.Num(), Audio.Num() - Offset);
		PendingAudio.Append(Audio.GetData() + Offset, Count);
		Offset += Count;
		if (PendingAudio.Num() == 1600)
		{
			TArray<float> Chunk = MoveTemp(PendingAudio);
			PendingAudio.Reset();
			if (!SendChunk(Chunk, false)) return;
		}
	}
}

void UJWNU_ScribeTranscriptor::CommitUtterance()
{
	// 서버 VAD 소유권은 고정이다. 수동 확정은 Finish 내부에서만 수행한다.
}

void UJWNU_ScribeTranscriptor::Receive(const FString& Json)
{
	TStrongObjectPtr<UJWNU_ScribeTranscriptor> KeepAlive(this);
	if (!bActive) return;
	TSharedPtr<FJsonObject> Event;
	FString Type;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Event) || !Event || !Event->TryGetStringField(TEXT("message_type"), Type))
	{ Fail(TEXT("Malformed Scribe event.")); return; }
	const uint32 Epoch = Generation;
	if (Type == TEXT("session_started"))
	{
		const TSharedPtr<FJsonObject>* Config = nullptr;
		FString Format, Model;
		double Rate = 0;
		if (bReady || !Event->TryGetObjectField(TEXT("config"), Config)
			|| !(*Config)->TryGetStringField(TEXT("audio_format"), Format) || Format != TEXT("pcm_16000")
			|| !(*Config)->TryGetStringField(TEXT("model_id"), Model) || Model != TEXT("scribe_v2_realtime")
			|| !(*Config)->TryGetNumberField(TEXT("sample_rate"), Rate) || Rate != 16000)
		{ Fail(TEXT("Scribe session configuration mismatch.")); return; }
		bReady = true;
		Deadline = 0;
		OnTranscriptionReadyNative.Broadcast();
		if (Generation == Epoch) OnTranscriptionReady.Broadcast();
		return;
	}
	if (Type == TEXT("partial_transcript") || Type == TEXT("committed_transcript"))
	{
		FJWNU_TranscriptUpdate Update;
		if (!bReady || !Event->TryGetStringField(TEXT("text"), Update.Text) || Update.Text.Len() > 16384)
		{ Fail(TEXT("Invalid Scribe transcript.")); return; }
		Update.SegmentId = LexToString(Segment);
		Update.bFinal = Type == TEXT("committed_transcript");
		// Scribe의 스트림 구간은 committed 이벤트로 닫힌다. 원격 GPT item ID/ACK를 요구하지 않는다.
		if (Update.bFinal)
		{
			const bool bEmptySegment = Update.Text.IsEmpty() && PartialText.IsEmpty();
			++Segment;
			PartialText.Reset();
			// 무음/종료 Commit의 빈 구간이 직전 UI 확정문을 지우지 않게 한다.
			if (bEmptySegment) return;
		}
		else PartialText = Update.Text;
		OnTranscriptionUpdateNative.Broadcast(Update);
		if (Generation == Epoch) OnTranscriptionUpdate.Broadcast(Update);
		return;
	}
	if (Type == TEXT("warning") || Type == TEXT("committed_transcript_with_timestamps") || Type == TEXT("committed_transcript_entities") || Type == TEXT("edited_transcript")) return;
	// 원격 오류 본문은 키/입력 반사를 포함할 수 있으므로 허용한 코드만 전달한다.
	static const TArray<FString> Errors = {TEXT("error"), TEXT("auth_error"), TEXT("quota_exceeded"), TEXT("throttled"), TEXT("commit_throttled"), TEXT("unaccepted_terms"), TEXT("unaccepted_terms_error"), TEXT("rate_limited"), TEXT("queue_overflow"), TEXT("resource_exhausted"), TEXT("session_time_limit_exceeded"), TEXT("input_error"), TEXT("invalid_request"), TEXT("chunk_size_exceeded"), TEXT("insufficient_audio_activity"), TEXT("transcriber_error")};
	if (Errors.Contains(Type)) Fail(TEXT("Scribe: ") + Type);
}

void UJWNU_ScribeTranscriptor::Finish()
{
	check(IsInGameThread());
	TStrongObjectPtr<UJWNU_ScribeTranscriptor> KeepAlive(this);
	if (!bActive || bFinishing) return;
	if (!bReady || (SentSamples == 0 && PendingAudio.IsEmpty())) { Complete(); return; }
	bFinishing = true;
	if (!PendingAudio.IsEmpty())
	{
		TArray<float> Tail = MoveTemp(PendingAudio);
		PendingAudio.Reset();
		if (!SendChunk(Tail, false)) return;
	}
	// 최초 입력이 짧으면 2초까지 무음을 보충해 초기 처리 대기 중인 발화를 남기지 않는다.
	while (SentSamples < 32000)
	{
		TArray<float> Silence;
		Silence.SetNumZeroed(static_cast<int32>(FMath::Min<int64>(1600, 32000 - SentSamples)));
		if (!SendChunk(Silence, false)) return;
	}
	if (!SendChunk({}, true)) return;
	Deadline = FPlatformTime::Seconds() + Settings.FinishDrainSeconds;
}

void UJWNU_ScribeTranscriptor::Tick(float DeltaSeconds)
{
	TStrongObjectPtr<UJWNU_ScribeTranscriptor> KeepAlive(this);
	if (!GetWorld() || GetWorld()->bIsTearingDown) { Cancel(); return; }
	if (Deadline <= 0 || FPlatformTime::Seconds() < Deadline) return;
	if (!bFinishing) { Fail(TEXT("Scribe session readiness timed out.")); return; }
	if (!PartialText.IsEmpty()) { Fail(TEXT("Scribe Finish timed out with an uncommitted transcript.")); return; }
	// EOS ACK가 없으므로 첫 Final에서 닫지 않고 설정된 전체 수신 구간을 기다린다.
	Complete();
}

void UJWNU_ScribeTranscriptor::Cancel()
{
	check(IsInGameThread());
	++Generation;
	bActive = bReady = bFinishing = false;
	Deadline = 0;
	Segment = 1;
	SentSamples = 0;
	PartialText.Reset();
	PendingAudio.Reset();
	if (Socket)
	{
		Socket->OnTextMessageNative.RemoveAll(this);
		Socket->OnErrorNative.RemoveAll(this);
		Socket->OnClosedNative.RemoveAll(this);
		Socket->Close();
		Socket = nullptr;
	}
}

void UJWNU_ScribeTranscriptor::Complete()
{
	Cancel();
	const uint32 Epoch = Generation;
	OnTranscriptionFinishedNative.Broadcast();
	if (Generation == Epoch) OnTranscriptionFinished.Broadcast();
}

void UJWNU_ScribeTranscriptor::Fail(const FString& Message)
{
	TStrongObjectPtr<UJWNU_ScribeTranscriptor> KeepAlive(this);
	Cancel();
	const uint32 Epoch = Generation;
	OnTranscriptionErrorNative.Broadcast(Message);
	if (Generation == Epoch) OnTranscriptionError.Broadcast(Message);
}

void UJWNU_ScribeTranscriptor::BeginDestroy()
{
	Cancel();
	Super::BeginDestroy();
}
