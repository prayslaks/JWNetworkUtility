// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_HttpRequestJob.h"
#include "JWNU_JsonHttpJob.generated.h"

/** 원본 JSON HTTP 전송의 종료 원인이다. HTTP 오류 본문은 별도로 보존한다. */
enum class EJWNU_JsonHttpError : uint8 { None, Network, Timeout, Cancelled, ResponseLimit };

/** 공급자와 무관한 유한 HTTP 요청 정책이다. MaxRetries는 최초 전송 이후 횟수다. */
struct JWNETWORKUTILITY_API FJWNU_JsonHttpOptions
{
    double TotalTimeoutSeconds = 20;
    double AttemptTimeoutSeconds = 10;
    int32 MaxRetries = 2;
    double InitialRetrySeconds = .25;
    double MaxRetrySeconds = 4;
    int32 MaxResponseBytes = 2 * 1024 * 1024;
    TSet<int32> RetryStatuses {429, 500, 502, 503, 504, 529};
};

/** 본문과 소문자 헤더를 변형 없이 전달하는 결과다. */
struct JWNETWORKUTILITY_API FJWNU_JsonHttpResult
{
    EJWNU_JsonHttpError Error = EJWNU_JsonHttpError::None;
    int32 StatusCode = 0;
    int32 Attempts = 0;
    FString Body;
    TMap<FString, FString> Headers;
};

DECLARE_DELEGATE_OneParam(FJWNU_JsonHttpCompleted, const FJWNU_JsonHttpResult&);
struct FJWNU_JsonHttpReceive;

/** 소유자가 GC 참조와 Pump를 관리하는 일회용 HTTP Job이다. 기존 HTTP/SSE 정책은 바꾸지 않는다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_JsonHttpJob : public UJWNU_HttpRequestJob
{
    GENERATED_BODY()
public:
    /** Initialize 이후 전송 제한과 완료 콜백을 지정하는 함수. */
    void Configure(const FJWNU_JsonHttpOptions& InOptions, FJWNU_JsonHttpCompleted InCompleted);
    virtual bool Execute() override;
    virtual void Cancel() override;
    virtual bool IsRunning() const override { return bActive; }
    virtual bool IsCancelled() const override { return Result.Error == EJWNU_JsonHttpError::Cancelled; }
    virtual void BeginDestroy() override;
    /** 게임 스레드에서 완료·재시도·실시간 제한을 처리하는 함수. */
    void Pump();
private:
    void SendAttempt();
    void StopTransport();
    void Finish(EJWNU_JsonHttpError Error);
    FJWNU_JsonHttpOptions Options;
    FJWNU_JsonHttpCompleted Completed;
    FJWNU_JsonHttpResult Result;
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request;
    TSharedPtr<FJWNU_JsonHttpReceive, ESPMode::ThreadSafe> Receive;
    double Deadline = 0, AttemptDeadline = 0, RetryAt = 0;
    bool bActive = false, bUsed = false;
};
