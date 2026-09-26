// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_ApiRequest.h"
#include "JWNU_GIS_ApiClientService.h"
#include "JWNU_HttpRequestJobHandle.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

UJWNU_ApiRequest* UJWNU_ApiRequest::CreateApiRequest(const UObject* WorldContextObject)
{
    check(IsInGameThread());
    auto* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || World->bIsTearingDown || !World->GetGameInstance()) { return nullptr; }
    auto* Client = World->GetGameInstance()->GetSubsystem<UJWNU_GIS_ApiClientService>();
    if (!Client || Client->bStoppingRequests) { return nullptr; }
    auto* Request = NewObject<UJWNU_ApiRequest>(Client);
    Request->OwnerClient = Client; Request->OwnerWorld = World;
    return Request;
}

bool UJWNU_ApiRequest::Start(EJWNU_ServiceType ServiceType, EJWNU_HttpMethod Method, const FString& Endpoint, const FString& ContentBody, const TMap<FString, FString>& QueryParams, bool bRequiresAuth)
{
    check(IsInGameThread());
    if (GetState() != EJWNU_RequestState::Created || !OwnerClient.IsValid() || !OwnerClient->TrackRequest(this)) { return false; }
    TStrongObjectPtr<UJWNU_ApiRequest> KeepAlive(this);
    ActivateRequest();
    auto* StartedHandle = UJWNU_GIS_ApiClientService::CallApi_NoTemplate(OwnerWorld.Get(), Method, ServiceType, Endpoint, ContentBody, QueryParams,
        FOnHttpResponseDelegate::CreateUObject(this, &UJWNU_ApiRequest::Receive),
        FOnHttpRequestJobRetryDelegate::CreateUObject(this, &UJWNU_ApiRequest::Retry), bRequiresAuth);
    Handle = StartedHandle;
    // 시작 내부의 동기 오류·이벤트에서 취소될 수 있으므로 반환된 전송도 정리한다.
    if (!IsActive() && Handle) { Handle->Cancel(); }
    if (!Handle && IsActive())
    {
        Receive(EJWNU_HttpStatusCode::None, TEXT("{\"success\":false,\"code\":\"START_FAILED\",\"message\":\"Cannot start API request\"}"));
    }
    return StartedHandle != nullptr;
}

void UJWNU_ApiRequest::Receive(EJWNU_HttpStatusCode Status, const FString& Body)
{
    if (!IsActive()) { return; }
    const bool bSuccess = Status == EJWNU_HttpStatusCode::OK || Status == EJWNU_HttpStatusCode::Created
        || Status == EJWNU_HttpStatusCode::Accepted || Status == EJWNU_HttpStatusCode::NoContent || Status == EJWNU_HttpStatusCode::OtherSuccess;
    if (bSuccess) { Result.StatusCode = Status; Result.ResponseBody = Body; }
    else
    {
        Error.Code = EJWNU_ApiRequestError::RequestFailed;
        Error.Response.StatusCode = Status; Error.Response.ResponseBody = Body;
    }
    Finish();
}

void UJWNU_ApiRequest::Retry(int32 AttemptNumber)
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_ApiRequest> KeepAlive(this);
    OnRetryNative.Broadcast(AttemptNumber);
    if (IsActive()) { OnRetry.Broadcast(AttemptNumber); }
}

void UJWNU_ApiRequest::CancelRequest()
{
    check(IsInGameThread());
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_ApiRequest> KeepAlive(this);
    Error.Code = EJWNU_ApiRequestError::Cancelled;
    if (Handle) { Handle->Cancel(); }
    Finish();
}

void UJWNU_ApiRequest::Finish()
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_ApiRequest> KeepAlive(this);
    SetFinishedState(Error.Code == EJWNU_ApiRequestError::None ? EJWNU_RequestState::Succeeded : (Error.Code == EJWNU_ApiRequestError::Cancelled ? EJWNU_RequestState::Cancelled : EJWNU_RequestState::Failed));
    if (Error.Code == EJWNU_ApiRequestError::None)
    { OnCompletedNative.Broadcast(Result); OnCompleted.Broadcast(Result); }
    else { OnFailedNative.Broadcast(Error); OnFailed.Broadcast(Error); }
    BroadcastFinished();
}
