// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_HttpRequest.h"
#include "JWNU_GIS_HttpClientHelper.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

UJWNU_HttpRequest* UJWNU_HttpRequest::CreateHttpRequest(const UObject* WorldContextObject)
{
    check(IsInGameThread());
    auto* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || World->bIsTearingDown || !World->GetGameInstance()) { return nullptr; }
    auto* Client = World->GetGameInstance()->GetSubsystem<UJWNU_GIS_HttpClientHelper>();
    if (!Client || Client->bStoppingRequests) { return nullptr; }
    auto* Request = NewObject<UJWNU_HttpRequest>(Client);
    Request->OwnerClient = Client; Request->OwnerWorld = World;
    return Request;
}

bool UJWNU_HttpRequest::Start(EJWNU_HttpMethod Method, const FString& URL, const FString& ContentBody, const TMap<FString, FString>& QueryParams, const FString& ApiKey)
{
    check(IsInGameThread());
    if (GetState() != EJWNU_RequestState::Created || !OwnerClient.IsValid() || !OwnerClient->TrackRequest(this)) { return false; }
    TStrongObjectPtr<UJWNU_HttpRequest> KeepAlive(this);
    ActivateRequest();
    const int32 SchemeLength = URL.StartsWith(TEXT("https://")) ? 8 : (URL.StartsWith(TEXT("http://")) ? 7 : 0);
    if (!SchemeLength || URL.Len() <= SchemeLength || URL[SchemeLength] == TEXT('/') || URL.Contains(TEXT("\r")) || URL.Contains(TEXT("\n"))
        || ApiKey.Contains(TEXT("\r")) || ApiKey.Contains(TEXT("\n")))
    {
        Error.Code = EJWNU_HttpRequestError::RequestFailed;
        Error.Message = TEXT("An absolute HTTP(S) URL and a valid API key header are required.");
        Finish(); return false;
    }
    auto* StartedJob = UJWNU_GIS_HttpClientHelper::SendRequest_RawResponse(OwnerWorld.Get(), Method, URL, ApiKey, ContentBody, QueryParams,
        FOnHttpRequestCompletedDelegate::CreateUObject(this, &UJWNU_HttpRequest::Receive),
        FOnHttpRequestJobRetryDelegate::CreateUObject(this, &UJWNU_HttpRequest::Retry));
    Job = StartedJob;
    // 전송 시작 중 동기 오류·취소가 발생한 경우 반환된 Job도 정리한다.
    if (!IsActive() && Job) { Job->Cancel(); }
    if (!Job && IsActive())
    {
        Error.Code = EJWNU_HttpRequestError::RequestFailed;
        Error.Message = TEXT("Cannot start HTTP request.");
        Finish();
    }
    return StartedJob != nullptr;
}

void UJWNU_HttpRequest::Receive(int32 Status, const FString& Body)
{
    if (!IsActive()) { return; }
    if (Status >= 200 && Status < 300) { Result.StatusCode = Status; Result.ResponseBody = Body; }
    else
    {
        Error.Code = EJWNU_HttpRequestError::RequestFailed;
        Error.Response.StatusCode = Status; Error.Response.ResponseBody = Body;
    }
    Finish();
}

void UJWNU_HttpRequest::Retry(int32 AttemptNumber)
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_HttpRequest> KeepAlive(this);
    OnRetryNative.Broadcast(AttemptNumber);
    if (IsActive()) { OnRetry.Broadcast(AttemptNumber); }
}

void UJWNU_HttpRequest::CancelRequest()
{
    check(IsInGameThread());
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_HttpRequest> KeepAlive(this);
    Error.Code = EJWNU_HttpRequestError::Cancelled;
    // 전송 취소 중 동기 콜백이 들어와도 중복 종료를 막는다.
    SetFinishedState(EJWNU_RequestState::Cancelled);
    if (Job) { Job->Cancel(); }
    OnFailedNative.Broadcast(Error); OnFailed.Broadcast(Error);
    BroadcastFinished();
}

void UJWNU_HttpRequest::Finish()
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_HttpRequest> KeepAlive(this);
    SetFinishedState(Error.Code == EJWNU_HttpRequestError::None ? EJWNU_RequestState::Succeeded : (Error.Code == EJWNU_HttpRequestError::Cancelled ? EJWNU_RequestState::Cancelled : EJWNU_RequestState::Failed));
    if (Error.Code == EJWNU_HttpRequestError::None)
    { OnCompletedNative.Broadcast(Result); OnCompleted.Broadcast(Result); }
    else { OnFailedNative.Broadcast(Error); OnFailed.Broadcast(Error); }
    BroadcastFinished();
}
