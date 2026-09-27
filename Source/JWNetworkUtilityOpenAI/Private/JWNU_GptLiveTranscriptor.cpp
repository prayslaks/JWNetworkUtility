// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_GptLiveTranscriptor.h"

#include "JWNU_GIS_OpenAI.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "JWNU_OpenAITranscriptionSession.h"

DEFINE_LOG_CATEGORY_STATIC(LogJWNU_GptLiveTranscriptor, Log, All);

UJWNU_GptLiveTranscriptor* UJWNU_GptLiveTranscriptor::CreateGptLiveTranscriptor(const UObject* Context)
{
	check(IsInGameThread());
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || World->bIsTearingDown || !World->GetGameInstance()) return nullptr;
	auto* Client = World->GetGameInstance()->GetSubsystem<UJWNU_GIS_OpenAI>();
	if (!Client || Client->bStopping) return nullptr;
	auto* Result = NewObject<UJWNU_GptLiveTranscriptor>(Client);
	Result->OwnerWorld = World;
	Result->OwnerClient = Client;
	return Result;
}

UWorld* UJWNU_GptLiveTranscriptor::GetWorld() const { return OwnerWorld.Get(); }

bool UJWNU_GptLiveTranscriptor::Start(const FJWNU_OpenAITranscriptionOptions& Options, const FString& ApiKey)
{
	return StartInternal(Options, ApiKey, false);
}

bool UJWNU_GptLiveTranscriptor::StartFromEnvironment(const FJWNU_OpenAITranscriptionOptions& Options)
{
	return StartInternal(Options, FString(), true);
}

bool UJWNU_GptLiveTranscriptor::StartInternal(const FJWNU_OpenAITranscriptionOptions& Options, const FString& ApiKey, bool bEnvironment)
{
	check(IsInGameThread());
	TStrongObjectPtr<UJWNU_GptLiveTranscriptor> KeepAlive(this);
	if (bActive)
	{
		UE_LOG(LogJWNU_GptLiveTranscriptor, Warning, TEXT("[GPT.StartRejected] Backend=%s Reason=AlreadyActive"), *GetName());
		return false;
	}
	// 이전 실행의 콜백을 무효화한 뒤 이번 Start의 세대를 모든 바인딩에 고정한다.
	Cancel();
	if (!OwnerClient.IsValid() || !OwnerClient->Track(this))
	{
		Fail(TEXT("GPT 전사기를 시작할 월드가 없습니다.")); return false;
	}
	bActive = true;
	const uint32 Epoch = Generation;
	Session = UJWNU_OpenAITranscriptionSession::CreateOpenAITranscriptionSession(this);
	if (!Session) { Fail(TEXT("GPT 전사 세션을 생성하지 못했습니다.")); return false; }
	Session->OnReadyNative.AddWeakLambda(this, [this, Epoch](const FString&)
	{
		if (Generation != Epoch) return;
		bReady = true;
		UE_LOG(LogJWNU_GptLiveTranscriptor, Log, TEXT("[GPT.Ready] Backend=%s Generation=%u"), *GetName(), Epoch);
		TStrongObjectPtr<UJWNU_GptLiveTranscriptor> KeepAlive(this);
		OnReadyNative.Broadcast();
		if (Generation == Epoch) OnReady.Broadcast();
	});
	Session->OnTranscriptNative.AddWeakLambda(this, [this, Epoch](const FJWNU_OpenAITranscript& Value)
	{
		if (Generation == Epoch) Transcript(Value);
	});
	Session->OnCommittedNative.AddWeakLambda(this, [this, Epoch](const FJWNU_OpenAITranscriptionCommit& Value)
	{
		if (Generation == Epoch) Committed(Value);
	});
	Session->OnErrorNative.AddWeakLambda(this, [this, Epoch](const FJWNU_OpenAITranscriptionError& Error)
	{
		// 실패한 발화를 건너뛰어 최종 결과 순서에 누락이 생기지 않도록 전체 세션을 중단한다.
		if (Generation == Epoch) Fail(Error.Code + TEXT(": ") + Error.Message);
	});
	Session->OnClosedNative.AddWeakLambda(this, [this, Epoch](const FJWNU_OpenAITranscriptionClose& Info)
	{
		if (Generation != Epoch) return;
		if (!Info.bFinalized) { Fail(Info.Reason); return; }
		if (!AwaitingCommits.IsEmpty() || !CommitOrder.IsEmpty() || !Items.IsEmpty())
		{ Fail(TEXT("GPT 전사 결과가 남은 상태에서 연결이 종료되었습니다.")); return; }
		UE_LOG(LogJWNU_GptLiveTranscriptor, Log, TEXT("[GPT.Closed] Backend=%s Generation=%u PendingCommits=%d PendingFinals=%d"), *GetName(), Epoch, AwaitingCommits.Num(), CommitOrder.Num());
		bActive = bReady = bClosing = false;
		EmitFinished();
	});
	UE_LOG(LogJWNU_GptLiveTranscriptor, Log, TEXT("[GPT.Start] Backend=%s Generation=%u AuthSource=%s"), *GetName(), Epoch,
		bEnvironment ? TEXT("Environment") : ApiKey.IsEmpty() ? TEXT("None") : TEXT("Explicit"));
	const bool bStarted = bEnvironment ? Session->StartFromEnvironment(Options) : Session->Start(Options, ApiKey);
	if (!bStarted && Generation == Epoch) Fail(TEXT("GPT 전사 연결을 시작하지 못했습니다."));
	return bStarted && Generation == Epoch && bActive;
}

TArray<uint8> UJWNU_GptLiveTranscriptor::EncodePCM(const TArray<float>& Audio, int32 Offset, int32 Count)
{
	TArray<uint8> Bytes;
	Bytes.SetNumUninitialized(Count * 2);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Value = FMath::IsFinite(Audio[Offset + Index]) ? FMath::Clamp(Audio[Offset + Index], -1.0f, 1.0f) : 0.0f;
		const int16 Sample = static_cast<int16>(FMath::RoundToInt(Value * (Value < 0 ? 32768.0f : 32767.0f)));
		Bytes[Index * 2] = static_cast<uint8>(Sample & 0xff);
		Bytes[Index * 2 + 1] = static_cast<uint8>((static_cast<uint16>(Sample) >> 8) & 0xff);
	}
	return Bytes;
}

void UJWNU_GptLiveTranscriptor::AppendAudio(const TArray<float>& MonoPCM)
{
	check(IsInGameThread());
	TStrongObjectPtr<UJWNU_GptLiveTranscriptor> KeepAlive(this);
	if (!bReady || bClosing || !Session)
	{
		UE_LOG(LogJWNU_GptLiveTranscriptor, Verbose, TEXT("[GPT.AudioSkipped] Backend=%s Ready=%d Closing=%d SessionValid=%d Samples=%d"), *GetName(), bReady, bClosing, Session != nullptr, MonoPCM.Num());
		return;
	}
	if (MonoPCM.Num() > GetSampleRate() * 30 - BufferedSamples) { Fail(TEXT("발화가 30초를 초과했습니다.")); return; }
	const uint32 Epoch = Generation;
	for (int32 Offset = 0; Offset < MonoPCM.Num(); Offset += 2400)
	{
		const int32 Count = FMath::Min(2400, MonoPCM.Num() - Offset);
		const bool bSent = Session->AppendInputAudio(EncodePCM(MonoPCM, Offset, Count));
		if (Generation != Epoch) return;
		if (!bSent) { Fail(TEXT("GPT 오디오 전송이 거절되었습니다.")); return; }
		BufferedSamples += Count;
	}
	UE_LOG(LogJWNU_GptLiveTranscriptor, Verbose, TEXT("[GPT.AudioSent] Backend=%s Generation=%u Samples=%d BufferedSamples=%d"), *GetName(), Generation, MonoPCM.Num(), BufferedSamples);
}

void UJWNU_GptLiveTranscriptor::CommitUtterance()
{
	check(IsInGameThread());
	TStrongObjectPtr<UJWNU_GptLiveTranscriptor> KeepAlive(this);
	if (!bReady || bClosing || !Session || BufferedSamples == 0)
	{
		UE_LOG(LogJWNU_GptLiveTranscriptor, Verbose, TEXT("[GPT.CommitSkipped] Backend=%s Ready=%d Closing=%d SessionValid=%d Samples=%d"), *GetName(), bReady, bClosing, Session != nullptr, BufferedSamples);
		return;
	}
	const uint32 Epoch = Generation;
	// API의 100ms 최소 길이를 맞추되 짧은 마지막 발화를 버리지 않는다.
	if (BufferedSamples < 2400)
	{
		TArray<float> Silence;
		Silence.SetNumZeroed(2400 - BufferedSamples);
		AppendAudio(Silence);
		if (Generation != Epoch) return;
	}
	AwaitingCommits.Add(FPlatformTime::Seconds() + 60.0);
	UE_LOG(LogJWNU_GptLiveTranscriptor, Log, TEXT("[GPT.CommitRequest] Backend=%s Generation=%u Samples=%d PendingCommits=%d"), *GetName(), Generation, BufferedSamples, AwaitingCommits.Num());
	const bool bCommitted = Session->CommitInputAudio();
	if (Generation != Epoch) return;
	if (!bCommitted) { Fail(TEXT("GPT 발화 확정이 거절되었습니다.")); return; }
	BufferedSamples = 0;
}

void UJWNU_GptLiveTranscriptor::Committed(const FJWNU_OpenAITranscriptionCommit& Commit)
{
	UE_LOG(LogJWNU_GptLiveTranscriptor, Log, TEXT("[GPT.CommitAck] Backend=%s Generation=%u Item=%s Previous=%s ExpectedPrevious=%s PendingCommits=%d"), *GetName(), Generation, *Commit.ItemId, *Commit.PreviousItemId, *LastCommittedId, AwaitingCommits.Num());
	if (AwaitingCommits.IsEmpty() || Commit.ItemId.IsEmpty() || CommitOrder.Contains(Commit.ItemId)
		|| (!LastCommittedId.IsEmpty() && Commit.PreviousItemId != LastCommittedId))
	{
		Fail(TEXT("GPT 발화 순서가 유효하지 않습니다.")); return;
	}
	if (!Items.Contains(Commit.ItemId) && Items.Num() >= 33) { Fail(TEXT("GPT 전사 대기열이 가득 찼습니다.")); return; }
	Items.FindOrAdd(Commit.ItemId).Deadline = AwaitingCommits[0];
	AwaitingCommits.RemoveAt(0);
	// 동일 소켓의 commit ack 순서로 입력 순서를 고정하고 final 도착 순서와 분리한다.
	CommitOrder.Add(Commit.ItemId);
	LastCommittedId = Commit.ItemId;
	FlushFinals();
}

void UJWNU_GptLiveTranscriptor::Transcript(const FJWNU_OpenAITranscript& Update)
{
	if (!bReady)
	{
		UE_LOG(LogJWNU_GptLiveTranscriptor, Verbose, TEXT("[GPT.ResultDiscarded] Backend=%s Item=%s Reason=NotReady"), *GetName(), *Update.ItemId);
		return;
	}
	if (!Items.Contains(Update.ItemId) && Items.Num() >= 33) { Fail(TEXT("GPT 부분 결과 대기열이 가득 찼습니다.")); return; }
	FItem& Item = Items.FindOrAdd(Update.ItemId);
	if (Item.bFinal)
	{
		UE_LOG(LogJWNU_GptLiveTranscriptor, Verbose, TEXT("[GPT.ResultDiscarded] Backend=%s Item=%s Reason=AlreadyFinal"), *GetName(), *Update.ItemId);
		return;
	}
	if (Update.bFinal) Item.Text = Update.Transcript;
	else Item.Text += Update.Delta;
	if (Item.Text.Len() > 16384) { Fail(TEXT("GPT 발화 텍스트 상한을 초과했습니다.")); return; }
	Item.bFinal = Update.bFinal;
	UE_LOG(LogJWNU_GptLiveTranscriptor, Verbose, TEXT("[GPT.ResultReceived] Backend=%s Generation=%u Item=%s Final=%d Chars=%d"), *GetName(), Generation, *Update.ItemId, Update.bFinal, Item.Text.Len());
	if (Update.bFinal) { FlushFinals(); return; }
	FJWNU_GptLiveTranscript Result;
	Result.UtteranceId = Update.ItemId;
	Result.Text = Item.Text;
	Result.Delta = Update.Delta;
	EmitTranscript(Result);
}

void UJWNU_GptLiveTranscriptor::FlushFinals()
{
	const uint32 Epoch = Generation;
	while (!CommitOrder.IsEmpty())
	{
		const FItem* Item = Items.Find(CommitOrder[0]);
		if (!Item || !Item->bFinal)
		{
			UE_LOG(LogJWNU_GptLiveTranscriptor, Verbose, TEXT("[GPT.FinalWaiting] Backend=%s Generation=%u WaitingFor=%s PendingFinals=%d"), *GetName(), Generation, *CommitOrder[0], CommitOrder.Num());
			return;
		}
		FJWNU_GptLiveTranscript Result;
		Result.UtteranceId = CommitOrder[0];
		Result.Text = Item->Text;
		Result.bFinal = true;
		// 수신자가 콜백 안에서 Finish/Cancel할 수 있으므로 전달 전에 대기열에서 제거한다.
		Items.Remove(Result.UtteranceId);
		CommitOrder.RemoveAt(0);
		UE_LOG(LogJWNU_GptLiveTranscriptor, Log, TEXT("[GPT.Final] Backend=%s Generation=%u Item=%s Chars=%d Bound=%d PendingFinals=%d"), *GetName(), Generation, *Result.UtteranceId, Result.Text.Len(), (OnTranscriptNative.IsBound() || OnTranscript.IsBound()), CommitOrder.Num());
		EmitTranscript(Result);
		if (Generation != Epoch) return;
	}
}

void UJWNU_GptLiveTranscriptor::Finish()
{
	check(IsInGameThread());
	TStrongObjectPtr<UJWNU_GptLiveTranscriptor> KeepAlive(this);
	UE_LOG(LogJWNU_GptLiveTranscriptor, Log, TEXT("[GPT.Finish] Backend=%s Generation=%u Ready=%d Samples=%d PendingCommits=%d PendingFinals=%d"), *GetName(), Generation, bReady, BufferedSamples, AwaitingCommits.Num(), CommitOrder.Num());
	if (!bActive || bClosing) return;
	if (!bReady || !Session) { Cancel(); EmitFinished(); return; }
	const uint32 Epoch = Generation;
	CommitUtterance();
	if (Generation != Epoch) return;
	bClosing = true;
	const bool bClosed = Session->Close();
	if (!bClosed && Generation == Epoch) Fail(TEXT("GPT 전사 종료가 거절되었습니다."));
}

void UJWNU_GptLiveTranscriptor::Cancel()
{
	check(IsInGameThread());
	++Generation;
	bActive = bReady = bClosing = false;
	BufferedSamples = 0;
	Items.Reset(); CommitOrder.Reset(); AwaitingCommits.Reset();
	LastCommittedId.Reset();
	if (Session)
	{
		Session->OnReadyNative.RemoveAll(this);
		Session->OnTranscriptNative.RemoveAll(this);
		Session->OnCommittedNative.RemoveAll(this);
		Session->OnErrorNative.RemoveAll(this);
		Session->OnClosedNative.RemoveAll(this);
		Session->Cancel();
		Session = nullptr;
	}
}

void UJWNU_GptLiveTranscriptor::Fail(const FString& Message)
{
	check(IsInGameThread());
	TStrongObjectPtr<UJWNU_GptLiveTranscriptor> KeepAlive(this);
	UE_LOG(LogJWNU_GptLiveTranscriptor, Warning, TEXT("[GPT.Error] Backend=%s Generation=%u Ready=%d Closing=%d Samples=%d PendingCommits=%d PendingFinals=%d Items=%d Reason=%s"), *GetName(), Generation, bReady, bClosing, BufferedSamples, AwaitingCommits.Num(), CommitOrder.Num(), Items.Num(), *Message);
	Cancel();
	const uint32 Epoch = Generation;
	OnErrorNative.Broadcast(Message);
	if (Generation == Epoch) OnError.Broadcast(Message);
}

void UJWNU_GptLiveTranscriptor::Poll(double Now)
{
	for (double Deadline : AwaitingCommits)
	{
		if (Now >= Deadline) { Fail(TEXT("GPT 발화 확정 응답 시간이 초과되었습니다.")); return; }
	}
	for (const auto& Pair : Items)
	{
		if (Pair.Value.Deadline > 0 && Now >= Pair.Value.Deadline)
		{
			Fail(TEXT("GPT 최종 전사 시간이 초과되었습니다.")); return;
		}
	}
}

void UJWNU_GptLiveTranscriptor::BeginDestroy()
{
	Cancel();
	Super::BeginDestroy();
}

void UJWNU_GptLiveTranscriptor::EmitTranscript(const FJWNU_GptLiveTranscript& Result)
{
	TStrongObjectPtr<UJWNU_GptLiveTranscriptor> KeepAlive(this);
	const uint32 Epoch = Generation;
	OnTranscriptNative.Broadcast(Result);
	// 네이티브 수신자가 취소하거나 재시작했으면 이전 실행의 BP 이벤트를 전달하지 않는다.
	if (Generation == Epoch) OnTranscript.Broadcast(Result);
}

void UJWNU_GptLiveTranscriptor::EmitFinished()
{
	TStrongObjectPtr<UJWNU_GptLiveTranscriptor> KeepAlive(this);
	const uint32 Epoch = Generation;
	OnFinishedNative.Broadcast();
	if (Generation == Epoch) OnFinished.Broadcast();
}
