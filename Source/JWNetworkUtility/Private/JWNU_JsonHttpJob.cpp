// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_JsonHttpJob.h"
#include "JWNU_JsonHttpRetry.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ScopeLock.h"
#include "Containers/StringConv.h"
#include "UObject/StrongObjectPtr.h"

/** HTTP 스레드는 공유 버퍼만 만지고 UObject 콜백은 Pump에서 전달한다. */
struct FJWNU_JsonHttpReceive
{
    FCriticalSection Mutex;
    TArray<uint8> Bytes;
    TMap<FString, FString> Headers;
    int32 Limit = 0, HeaderBytes = 0, Status = 0;
    bool bClosed = false, bComplete = false, bSuccess = false, bOverflow = false;
};

void UJWNU_JsonHttpJob::Configure(const FJWNU_JsonHttpOptions& InOptions, FJWNU_JsonHttpCompleted InCompleted)
{
    check(IsInGameThread());
    check(!bUsed);
    Options = InOptions; Completed = MoveTemp(InCompleted);
}

bool UJWNU_JsonHttpJob::Execute()
{
    check(IsInGameThread());
    if (bUsed || !FMath::IsFinite(Options.TotalTimeoutSeconds) || Options.TotalTimeoutSeconds <= 0
        || !FMath::IsFinite(Options.AttemptTimeoutSeconds) || Options.AttemptTimeoutSeconds <= 0
        || !FMath::IsFinite(Options.InitialRetrySeconds) || Options.InitialRetrySeconds <= 0
        || !FMath::IsFinite(Options.MaxRetrySeconds) || Options.MaxRetrySeconds < Options.InitialRetrySeconds
        || Options.MaxRetries < 0 || Options.MaxRetries > 10 || Options.MaxResponseBytes <= 0) { return false; }
    bUsed = true; bActive = true;
    Deadline = FPlatformTime::Seconds() + Options.TotalTimeoutSeconds;
    SendAttempt();
    return true;
}

void UJWNU_JsonHttpJob::SendAttempt()
{
    ++Result.Attempts;
    AttemptDeadline = FPlatformTime::Seconds() + Options.AttemptTimeoutSeconds;
    Receive = MakeShared<FJWNU_JsonHttpReceive, ESPMode::ThreadSafe>();
    Receive->Limit = Options.MaxResponseBytes;
    const auto Buffer = Receive.ToSharedRef();
    Request = CreateConfiguredRequest();
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    Request->SetTimeout(0.f);
    Request->SetActivityTimeout(0.f);
    Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnHttpThread);
    Request->OnHeaderReceived().BindLambda([Buffer](FHttpRequestPtr, const FString& Name, const FString& Value)
    {
        FScopeLock Lock(&Buffer->Mutex);
        if (Buffer->bClosed || Buffer->bOverflow) { return; }
        const int64 Size = (static_cast<int64>(Name.Len()) + Value.Len()) * sizeof(TCHAR);
        if (Size > 16384 - Buffer->HeaderBytes) { Buffer->bOverflow = true; return; }
        Buffer->HeaderBytes += static_cast<int32>(Size);
        Buffer->Headers.Add(Name.ToLower(), Value);
    });
    const bool bStreaming = Request->SetResponseBodyReceiveStreamDelegateV2(FHttpRequestStreamDelegateV2::CreateLambda(
        [Buffer](void* Data, int64& Size)
        {
            FScopeLock Lock(&Buffer->Mutex);
            if (Buffer->bClosed || Buffer->bOverflow || Size < 0 || Size > Buffer->Limit - Buffer->Bytes.Num())
            { if (!Buffer->bClosed) { Buffer->bOverflow = true; } Size = 0; return; }
            if (Size) { Buffer->Bytes.Append(static_cast<const uint8*>(Data), static_cast<int32>(Size)); }
        }));
    Request->OnProcessRequestComplete().BindLambda([Buffer](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
    {
        FScopeLock Lock(&Buffer->Mutex);
        if (Buffer->bClosed) { return; }
        Buffer->bComplete = true; Buffer->bSuccess = bSuccess;
        Buffer->Status = Response.IsValid() ? Response->GetResponseCode() : 0;
    });
    if (!bStreaming || !Request->ProcessRequest())
    {
        FScopeLock Lock(&Buffer->Mutex);
        Buffer->bComplete = true; Buffer->bSuccess = false;
    }
}

void UJWNU_JsonHttpJob::Pump()
{
    check(IsInGameThread());
    if (!bActive) { return; }
    const double Now = FPlatformTime::Seconds();
    if (Now >= Deadline) { Finish(EJWNU_JsonHttpError::Timeout); return; }
    if (!Receive)
    {
        if (Now >= RetryAt) { SendAttempt(); }
        return;
    }
    bool bDone, bSuccess, bOverflow;
    {
        FScopeLock Lock(&Receive->Mutex);
        bDone = Receive->bComplete; bSuccess = Receive->bSuccess; bOverflow = Receive->bOverflow;
        if (bDone)
        {
            Result.StatusCode = Receive->Status;
            Result.Headers = MoveTemp(Receive->Headers);
            if (!Receive->Bytes.IsEmpty())
            {
                FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(Receive->Bytes.GetData()), Receive->Bytes.Num());
                Result.Body = FString(Text.Length(), Text.Get());
            }
            else { Result.Body.Reset(); }
        }
    }
    if (bOverflow) { Finish(EJWNU_JsonHttpError::ResponseLimit); return; }
    const bool bTimedOut = !bDone && Now >= AttemptDeadline;
    if (!bDone && !bTimedOut) { return; }
    if (bTimedOut) { Result.StatusCode = 0; Result.Body.Reset(); Result.Headers.Reset(); }
    const bool bRetry = bTimedOut || !bSuccess || Options.RetryStatuses.Contains(Result.StatusCode);
    if (bRetry && Result.Attempts <= Options.MaxRetries)
    {
        const double Backoff = FMath::Min(Options.MaxRetrySeconds, Options.InitialRetrySeconds * FMath::Pow(2., Result.Attempts - 1));
        const double Delay = JWNU::JsonHttp::RetryDelay(Result.Headers, Backoff * FMath::FRandRange(.8f, 1.f));
        if (Now + Delay < Deadline)
        {
            StopTransport(); RetryAt = Now + Delay; return;
        }
    }
    Finish(bTimedOut ? EJWNU_JsonHttpError::Timeout : (bSuccess ? EJWNU_JsonHttpError::None : EJWNU_JsonHttpError::Network));
}

void UJWNU_JsonHttpJob::StopTransport()
{
    if (Receive) { FScopeLock Lock(&Receive->Mutex); Receive->bClosed = true; }
    if (Request) { Request->CancelRequest(); Request.Reset(); }
    Receive.Reset();
}

void UJWNU_JsonHttpJob::Finish(EJWNU_JsonHttpError Error)
{
    if (!bActive) { return; }
    TStrongObjectPtr<UJWNU_JsonHttpJob> KeepAlive(this);
    bActive = false; Result.Error = Error;
    StopTransport();
    // 기본 Job에 보관했던 credential과 요청 본문은 종료 시 해제한다.
    Initialize(EJWNU_HttpMethod::Post, TEXT(""), TEXT(""), TEXT(""), {});
    auto Callback = MoveTemp(Completed); Completed.Unbind();
    Callback.ExecuteIfBound(Result);
}

void UJWNU_JsonHttpJob::Cancel()
{
    check(IsInGameThread());
    Finish(EJWNU_JsonHttpError::Cancelled);
}

void UJWNU_JsonHttpJob::BeginDestroy()
{
    Completed.Unbind(); bActive = false; StopTransport();
    Super::BeginDestroy();
}
