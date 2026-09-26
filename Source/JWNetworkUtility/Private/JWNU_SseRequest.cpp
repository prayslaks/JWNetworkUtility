// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_SseRequest.h"
#include "JWNU_GIS_SseClient.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

UJWNU_SseRequestBase* UJWNU_SseRequestBase::Create(const UObject* WorldContextObject, TSubclassOf<UJWNU_SseRequestBase> Class)
{
    check(IsInGameThread());
    auto* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    auto* Client = UJWNU_GIS_SseClient::Get(WorldContextObject);
    if (!World || World->bIsTearingDown || !Client || Client->bShuttingDown) { return nullptr; }
    auto* Request = NewObject<UJWNU_SseRequestBase>(Client, Class);
    Request->OwnerClient = Client; Request->OwnerWorld = World;
    return Request;
}

UJWNU_SseRequest* UJWNU_SseRequest::CreateSseRequest(const UObject* WorldContextObject)
{ return Cast<UJWNU_SseRequest>(Create(WorldContextObject, StaticClass())); }

UJWNU_SseApiRequest* UJWNU_SseApiRequest::CreateSseApiRequest(const UObject* WorldContextObject)
{ return Cast<UJWNU_SseApiRequest>(Create(WorldContextObject, StaticClass())); }

bool UJWNU_SseRequestBase::Begin()
{
    check(IsInGameThread());
    if (GetState() != EJWNU_RequestState::Created || !OwnerClient.IsValid() || !OwnerClient->TrackRequest(this)) { return false; }
    ActivateRequest(); return true;
}

bool UJWNU_SseRequest::Start(EJWNU_HttpMethod Method, const FString& URL, const FString& ContentBody,
    const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options, const FString& ApiKey)
{
    if (!Begin()) { return false; }
    TStrongObjectPtr<UJWNU_SseRequest> KeepAlive(this);
    return Attach(UJWNU_GIS_SseClient::SendSseRequest(OwnerWorld.Get(), Method, URL, ApiKey, ContentBody, QueryParams, Options, MakeCallbacks()));
}

bool UJWNU_SseApiRequest::Start(EJWNU_ServiceType ServiceType, EJWNU_HttpMethod Method, const FString& Endpoint, const FString& ContentBody,
    const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options, bool bRequiresAuth)
{
    if (!Begin()) { return false; }
    TStrongObjectPtr<UJWNU_SseApiRequest> KeepAlive(this);
    return Attach(UJWNU_GIS_SseClient::CallSseApi_NoTemplate(OwnerWorld.Get(), Method, ServiceType, Endpoint, ContentBody, QueryParams, Options, MakeCallbacks(), bRequiresAuth));
}

bool UJWNU_SseRequestBase::Attach(UJWNU_HttpRequestJobHandle* StartedHandle)
{
    Handle = StartedHandle;
    if (!IsActive() && Handle) { Handle->Cancel(); }
    if (!Handle && IsActive())
    {
        FJWNU_SseResponse Failure; Failure.Error = EJWNU_SseError::InvalidRequest; Failure.Message = TEXT("Cannot start SSE request.");
        ReceiveFailed(Failure);
    }
    return Handle != nullptr;
}

FJWNU_SseCallbacks UJWNU_SseRequestBase::MakeCallbacks()
{
    FJWNU_SseCallbacks Callbacks;
    Callbacks.OnOpened.BindUObject(this, &UJWNU_SseRequestBase::ReceiveOpened);
    Callbacks.OnEvent.BindUObject(this, &UJWNU_SseRequestBase::ReceiveEvent);
    Callbacks.OnCompleted.BindUObject(this, &UJWNU_SseRequestBase::ReceiveCompleted);
    Callbacks.OnError.BindUObject(this, &UJWNU_SseRequestBase::ReceiveFailed);
    Callbacks.OnCancelled.BindUObject(this, &UJWNU_SseRequestBase::ReceiveCancelled);
    return Callbacks;
}

void UJWNU_SseRequestBase::ReceiveOpened(const FJWNU_SseResponse& Response)
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_SseRequestBase> KeepAlive(this);
    OnOpenedNative.Broadcast(Response);
    if (IsActive()) { OnOpened.Broadcast(Response); }
}

void UJWNU_SseRequestBase::ReceiveEvent(const FJWNU_SseEvent& Event)
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_SseRequestBase> KeepAlive(this);
    OnEventNative.Broadcast(Event);
    if (IsActive()) { OnEvent.Broadcast(Event); }
}

void UJWNU_SseRequestBase::ReceiveCompleted(const FJWNU_SseResponse& Response)
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_SseRequestBase> KeepAlive(this);
    Result = Response; SetFinishedState(EJWNU_RequestState::Succeeded);
    OnCompletedNative.Broadcast(Result); OnCompleted.Broadcast(Result); BroadcastFinished();
}

void UJWNU_SseRequestBase::ReceiveFailed(const FJWNU_SseResponse& Response)
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_SseRequestBase> KeepAlive(this);
    Error = Response; SetFinishedState(Response.Error == EJWNU_SseError::Cancelled ? EJWNU_RequestState::Cancelled : EJWNU_RequestState::Failed);
    OnFailedNative.Broadcast(Error); OnFailed.Broadcast(Error); BroadcastFinished();
}

void UJWNU_SseRequestBase::ReceiveCancelled()
{
    FJWNU_SseResponse Failure; Failure.Error = EJWNU_SseError::Cancelled;
    Failure.Message = TEXT("SSE request cancelled."); ReceiveFailed(Failure);
}

void UJWNU_SseRequestBase::CancelRequest()
{
    check(IsInGameThread());
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_SseRequestBase> KeepAlive(this);
    if (Handle) { Handle->Cancel(); }
    if (IsActive()) { ReceiveCancelled(); }
}
