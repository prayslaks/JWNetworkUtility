// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_BFL_SseClient.h"
#include "JWNU_SseRequest.h"
#include "UObject/StrongObjectPtr.h"

UJWNU_SseApiRequest* UJWNU_BFL_SseClient::CallSseApi(const UObject* WorldContextObject, EJWNU_HttpMethod Method,
	const FString& Body, const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options,
	const FJWNU_OnSseResponseBP& OnOpened, const FJWNU_OnSseEventBP& OnEvent,
	const FJWNU_OnSseResponseBP& OnCompleted, const FJWNU_OnSseResponseBP& OnError,
	const FJWNU_OnSseCancelledBP& OnCancelled, EJWNU_ServiceType ServiceType, const FString& Endpoint, bool bRequiresAuth)
{
	TStrongObjectPtr<UJWNU_SseApiRequest> Request(UJWNU_SseApiRequest::CreateSseApiRequest(WorldContextObject));
	if (!Request.IsValid())
	{
		FJWNU_SseResponse Failure;
		Failure.Error = EJWNU_SseError::InvalidRequest;
		Failure.Message = TEXT("Cannot create SSE API request for this world.");
		OnError.ExecuteIfBound(Failure);
		return nullptr;
	}
	Request->OnOpenedNative.AddLambda([OnOpened](const FJWNU_SseResponse& Response) { OnOpened.ExecuteIfBound(Response); });
	Request->OnEventNative.AddLambda([OnEvent](const FJWNU_SseEvent& Event) { OnEvent.ExecuteIfBound(Event); });
	Request->OnCompletedNative.AddLambda([OnCompleted](const FJWNU_SseResponse& Response) { OnCompleted.ExecuteIfBound(Response); });
	Request->OnFailedNative.AddLambda([OnError, OnCancelled](const FJWNU_SseResponse& Response)
	{
		// 즉시 실행 노드는 기존 취소 전용 콜백을 유지하며 오류 콜백과 중복 통지하지 않는다.
		if (Response.Error == EJWNU_SseError::Cancelled) { OnCancelled.ExecuteIfBound(); }
		else { OnError.ExecuteIfBound(Response); }
	});
	Request->Start(ServiceType, Method, Endpoint, Body, QueryParams, Options, bRequiresAuth);
	return Request.Get();
}
