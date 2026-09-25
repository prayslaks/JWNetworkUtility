// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNU_HttpRequestJob.h"
#include "JWNU_SseParser.h"
#include "JWNU_SseRequestJob.generated.h"

struct FJWNU_SseReceiveQueue;

/** 공통 HTTP 요청 설정과 기존 Handle을 사용하는 SSE 전용 Job이다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_SseRequestJob : public UJWNU_HttpRequestJob
{
	GENERATED_BODY()
public:
	/** 요청 시작 전 스트림 옵션과 콜백을 지정하는 함수. */
	void ConfigureSse(const FJWNU_SseOptions& InOptions, const FJWNU_SseCallbacks& InCallbacks);
	virtual bool Execute() override;
	virtual void Cancel() override;
	virtual bool IsRunning() const override { return bStreaming; }
	virtual bool IsCancelled() const override { return bCancelled; }
	virtual void BeginDestroy() override;
	/** 게임 스레드에서 수신 큐와 제한 시간을 처리하는 함수. */
	void Pump();

private:
	bool OpenStream();
	void Fail(EJWNU_SseError Error, const FString& Message);
	void StopTransport();
	FJWNU_SseOptions Options;
	FJWNU_SseCallbacks Callbacks;
	FJWNU_SseParser Parser;
	FJWNU_SseResponse Response;
	TArray<uint8> ErrorBytes;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> StreamRequest;
	TSharedPtr<FJWNU_SseReceiveQueue, ESPMode::ThreadSafe> Queue;
	double StartedSeconds = 0;
	bool bStreaming = false;
	bool bCancelled = false;
	bool bOpened = false;
};
