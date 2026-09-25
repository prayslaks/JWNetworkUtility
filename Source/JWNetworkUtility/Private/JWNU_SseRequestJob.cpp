// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_SseRequestJob.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ScopeLock.h"
#include "Containers/StringConv.h"

/** HTTP 스레드가 UObject에 접근하지 않고 수신 순서를 보존하는 큐다. */
struct FJWNU_SseReceiveQueue
{
	struct FItem
	{
		enum class EKind : uint8 { Status, Header, Bytes, Complete } Kind;
		int32 Status = 0;
		FString Name, Value;
		TArray<uint8> Bytes;
		bool bSuccess = false;
	};
	FCriticalSection Mutex;
	TArray<FItem> Items;
	int64 BufferedBytes = 0;
	int32 MaxBytes = 0;
	double LastReceiveSeconds = 0;
	bool bOverflow = false;
	bool bClosed = false;
	bool Reserve(const int64 Bytes)
	{
		if (bOverflow || Bytes > MaxBytes - BufferedBytes) { bOverflow = true; return false; }
		BufferedBytes += Bytes;
		return true;
	}
};

void UJWNU_SseRequestJob::ConfigureSse(const FJWNU_SseOptions& InOptions, const FJWNU_SseCallbacks& InCallbacks)
{
	Options = InOptions; Callbacks = InCallbacks;
}

bool UJWNU_SseRequestJob::Execute()
{
	check(IsInGameThread());
	if (bStreaming) { return false; }
	bStreaming = true; bCancelled = false; bOpened = false;
	Response = FJWNU_SseResponse(); ErrorBytes.Reset();
	StartedSeconds = FPlatformTime::Seconds();
	Parser.Reset(Options.MaxEventBytes);
	Queue = MakeShared<FJWNU_SseReceiveQueue, ESPMode::ThreadSafe>();
	Queue->MaxBytes = FMath::Max(1, Options.MaxQueuedBytes);
	Queue->LastReceiveSeconds = StartedSeconds;
	StreamRequest = CreateConfiguredRequest();
	StreamRequest->SetHeader(TEXT("Accept"), TEXT("text/event-stream"));
	for (const auto& Pair : Options.Headers) { StreamRequest->SetHeader(Pair.Key, Pair.Value); }
	// 시간 제한은 Pump가 실제 시간으로 관리해 장시간 스트림에 일반 HTTP 상한을 적용하지 않는다.
	StreamRequest->SetTimeout(0.f);
	StreamRequest->SetActivityTimeout(0.f);
	StreamRequest->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnHttpThread);
	const auto Receive = Queue.ToSharedRef();
	StreamRequest->OnStatusCodeReceived().BindLambda([Receive](FHttpRequestPtr, int32 Status)
	{
		FScopeLock Lock(&Receive->Mutex);
		if (Receive->bClosed || !Receive->Reserve(sizeof(FJWNU_SseReceiveQueue::FItem))) { return; }
		auto& Item = Receive->Items.AddDefaulted_GetRef(); Item.Kind = FJWNU_SseReceiveQueue::FItem::EKind::Status; Item.Status = Status;
	});
	StreamRequest->OnHeaderReceived().BindLambda([Receive](FHttpRequestPtr, const FString& Name, const FString& Value)
	{
		FScopeLock Lock(&Receive->Mutex);
		if (Receive->bClosed) { return; }
		Receive->LastReceiveSeconds = FPlatformTime::Seconds();
		if (!Receive->Reserve(sizeof(FJWNU_SseReceiveQueue::FItem) + (static_cast<int64>(Name.Len()) + Value.Len() + 2) * sizeof(TCHAR))) { return; }
		auto& Item = Receive->Items.AddDefaulted_GetRef(); Item.Kind = FJWNU_SseReceiveQueue::FItem::EKind::Header; Item.Name = Name; Item.Value = Value;
	});
	const bool bStreamSupported = StreamRequest->SetResponseBodyReceiveStreamDelegateV2(FHttpRequestStreamDelegateV2::CreateLambda(
		[Receive](void* Bytes, int64& Length)
		{
			FScopeLock Lock(&Receive->Mutex);
			if (Length == 0) { return; }
			if (Receive->bClosed || Length < 0 || Length > Receive->MaxBytes)
			{
				if (!Receive->bClosed) { Receive->bOverflow = true; }
				Length = 0; return;
			}
			const bool bAppend = !Receive->Items.IsEmpty() && Receive->Items.Last().Kind == FJWNU_SseReceiveQueue::FItem::EKind::Bytes;
			if (!Receive->Reserve(Length + (bAppend ? 0 : sizeof(FJWNU_SseReceiveQueue::FItem)))) { Length = 0; return; }
			Receive->LastReceiveSeconds = FPlatformTime::Seconds();
			auto& Item = bAppend ? Receive->Items.Last() : Receive->Items.AddDefaulted_GetRef(); Item.Kind = FJWNU_SseReceiveQueue::FItem::EKind::Bytes;
			Item.Bytes.Append(static_cast<const uint8*>(Bytes), static_cast<int32>(Length));
		}));
	StreamRequest->OnProcessRequestComplete().BindLambda([Receive](FHttpRequestPtr, FHttpResponsePtr HttpResponse, bool bSuccess)
	{
		FScopeLock Lock(&Receive->Mutex);
		if (Receive->bClosed || !Receive->Reserve(sizeof(FJWNU_SseReceiveQueue::FItem))) { return; }
		auto& Item = Receive->Items.AddDefaulted_GetRef(); Item.Kind = FJWNU_SseReceiveQueue::FItem::EKind::Complete;
		Item.Status = HttpResponse.IsValid() ? HttpResponse->GetResponseCode() : 0; Item.bSuccess = bSuccess;
	});
	if (!bStreamSupported || !StreamRequest->ProcessRequest())
	{
		// 실행 실패도 다음 Pump에서 전달하여 호출자가 Handle을 먼저 받게 한다.
		FScopeLock Lock(&Receive->Mutex);
		auto& Item = Receive->Items.AddDefaulted_GetRef(); Item.Kind = FJWNU_SseReceiveQueue::FItem::EKind::Complete;
		Item.Value = TEXT("HTTP streaming request could not start");
	}
	return true;
}

bool UJWNU_SseRequestJob::OpenStream()
{
	if (bOpened) { return true; }
	if (Response.StatusCode < 200 || Response.StatusCode >= 300) { return false; }
	FString Mime = Response.Headers.FindRef(TEXT("content-type"));
	int32 Semicolon = INDEX_NONE;
	if (Mime.FindChar(';', Semicolon)) { Mime.LeftInline(Semicolon); }
	if (!Mime.TrimStartAndEnd().Equals(TEXT("text/event-stream"), ESearchCase::IgnoreCase))
	{
		Fail(EJWNU_SseError::InvalidContentType, TEXT("Expected text/event-stream")); return false;
	}
	bOpened = true;
	Callbacks.OnOpened.ExecuteIfBound(Response);
	return bStreaming;
}

void UJWNU_SseRequestJob::Pump()
{
	check(IsInGameThread());
	if (!bStreaming || !Queue) { return; }
	TArray<FJWNU_SseReceiveQueue::FItem> Items;
	double LastReceive;
	bool bOverflow;
	{
		FScopeLock Lock(&Queue->Mutex);
		Items = MoveTemp(Queue->Items); Queue->BufferedBytes = 0;
		LastReceive = Queue->LastReceiveSeconds; bOverflow = Queue->bOverflow;
	}
	if (bOverflow) { Fail(EJWNU_SseError::BufferLimit, TEXT("SSE receive queue exceeded its limit")); return; }
	for (auto& Item : Items)
	{
		if (!bStreaming) { return; }
		using EKind = FJWNU_SseReceiveQueue::FItem::EKind;
		if (Item.Kind == EKind::Status) { Response.StatusCode = Item.Status; }
		else if (Item.Kind == EKind::Header) { Response.Headers.Add(Item.Name.ToLower(), Item.Value); }
		else if (Item.Kind == EKind::Bytes)
		{
			if (Response.StatusCode < 200 || Response.StatusCode >= 300)
			{
				const int32 Copy = FMath::Min(Item.Bytes.Num(), FMath::Max(0, Options.MaxErrorBodyBytes - ErrorBytes.Num()));
				ErrorBytes.Append(Item.Bytes.GetData(), Copy);
				Response.bErrorBodyTruncated |= Copy < Item.Bytes.Num();
				continue;
			}
			if (!OpenStream()) { return; }
			TArray<FJWNU_SseEvent> Events;
			if (!Parser.Feed(Item.Bytes.GetData(), Item.Bytes.Num(), Events)) { Fail(EJWNU_SseError::BufferLimit, TEXT("SSE event exceeded its limit")); return; }
			for (const auto& Event : Events)
			{
				if (!bStreaming) { return; }
				Callbacks.OnEvent.ExecuteIfBound(Event);
			}
		}
		else if (Item.Kind == EKind::Complete)
		{
			Response.StatusCode = Item.Status;
			if (!Item.bSuccess) { Fail(EJWNU_SseError::Network, Item.Value.IsEmpty() ? TEXT("HTTP stream disconnected") : Item.Value); return; }
			if (Response.StatusCode < 200 || Response.StatusCode >= 300) { Fail(EJWNU_SseError::Http, TEXT("HTTP request rejected")); return; }
			if (!OpenStream()) { return; }
			Parser.Finish(); bStreaming = false; StopTransport();
			Callbacks.OnCompleted.ExecuteIfBound(Response); return;
		}
	}
	if (!bOpened && Response.StatusCode >= 200 && Response.StatusCode < 300 && Response.Headers.Contains(TEXT("content-type")))
	{
		if (!OpenStream()) { return; }
	}
	const double Now = FPlatformTime::Seconds();
	if ((Options.TotalTimeoutSeconds > 0 && Now - StartedSeconds >= Options.TotalTimeoutSeconds)
		|| (!bOpened && Options.OpenTimeoutSeconds > 0 && Now - StartedSeconds >= Options.OpenTimeoutSeconds)
		|| (bOpened && Options.IdleTimeoutSeconds > 0 && Now - LastReceive >= Options.IdleTimeoutSeconds))
	{
		Fail(EJWNU_SseError::Timeout, TEXT("SSE deadline or receive idle timeout exceeded"));
	}
}

void UJWNU_SseRequestJob::Fail(const EJWNU_SseError Error, const FString& Message)
{
	if (!bStreaming) { return; }
	bStreaming = false; Response.Error = Error; Response.Message = Message;
	if (!ErrorBytes.IsEmpty())
	{
		const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(ErrorBytes.GetData()), ErrorBytes.Num());
		Response.ErrorBody = FString(Converted.Length(), Converted.Get());
	}
	StopTransport();
	Callbacks.OnError.ExecuteIfBound(Response);
}

void UJWNU_SseRequestJob::StopTransport()
{
	if (Queue) { FScopeLock Lock(&Queue->Mutex); Queue->bClosed = true; Queue->Items.Reset(); }
	if (StreamRequest) { StreamRequest->CancelRequest(); StreamRequest.Reset(); }
}

void UJWNU_SseRequestJob::Cancel()
{
	if (!bStreaming) { return; }
	bStreaming = false; bCancelled = true; StopTransport();
	Callbacks.OnCancelled.ExecuteIfBound();
}

void UJWNU_SseRequestJob::BeginDestroy()
{
	StopTransport();
	Super::BeginDestroy();
}
