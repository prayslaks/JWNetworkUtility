// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_BFL_SseClient.h"

UJWNU_HttpRequestJobHandle* UJWNU_BFL_SseClient::SendSseRequest(const UObject* WorldContextObject, EJWNU_HttpMethod Method,
		const FString& Body, const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options,
		const FJWNU_OnSseResponseBP& OnOpened, const FJWNU_OnSseEventBP& OnEvent,
		const FJWNU_OnSseResponseBP& OnCompleted, const FJWNU_OnSseResponseBP& OnError,
		const FJWNU_OnSseCancelledBP& OnCancelled, const FString& URL, const FString& AuthToken)
{
	FJWNU_SseCallbacks Callbacks;
	Callbacks.OnOpened = FJWNU_OnSseResponse::CreateLambda([OnOpened](const FJWNU_SseResponse& Value) { OnOpened.ExecuteIfBound(Value); });
	Callbacks.OnEvent = FJWNU_OnSseEvent::CreateLambda([OnEvent](const FJWNU_SseEvent& Value) { OnEvent.ExecuteIfBound(Value); });
	Callbacks.OnCompleted = FJWNU_OnSseResponse::CreateLambda([OnCompleted](const FJWNU_SseResponse& Value) { OnCompleted.ExecuteIfBound(Value); });
	Callbacks.OnError = FJWNU_OnSseResponse::CreateLambda([OnError](const FJWNU_SseResponse& Value) { OnError.ExecuteIfBound(Value); });
	Callbacks.OnCancelled = FJWNU_OnSseCancelled::CreateLambda([OnCancelled]() { OnCancelled.ExecuteIfBound(); });
	return UJWNU_GIS_SseClient::SendSseRequest(WorldContextObject, Method, URL, AuthToken, Body, QueryParams, Options, Callbacks);
}

UJWNU_HttpRequestJobHandle* UJWNU_BFL_SseClient::CallSseApi(const UObject* WorldContextObject, EJWNU_HttpMethod Method,
		const FString& Body, const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options,
		const FJWNU_OnSseResponseBP& OnOpened, const FJWNU_OnSseEventBP& OnEvent,
		const FJWNU_OnSseResponseBP& OnCompleted, const FJWNU_OnSseResponseBP& OnError,
		const FJWNU_OnSseCancelledBP& OnCancelled, EJWNU_ServiceType ServiceType, const FString& Endpoint, bool bRequiresAuth)
{
	FJWNU_SseCallbacks Callbacks;
	Callbacks.OnOpened = FJWNU_OnSseResponse::CreateLambda([OnOpened](const FJWNU_SseResponse& Value) { OnOpened.ExecuteIfBound(Value); });
	Callbacks.OnEvent = FJWNU_OnSseEvent::CreateLambda([OnEvent](const FJWNU_SseEvent& Value) { OnEvent.ExecuteIfBound(Value); });
	Callbacks.OnCompleted = FJWNU_OnSseResponse::CreateLambda([OnCompleted](const FJWNU_SseResponse& Value) { OnCompleted.ExecuteIfBound(Value); });
	Callbacks.OnError = FJWNU_OnSseResponse::CreateLambda([OnError](const FJWNU_SseResponse& Value) { OnError.ExecuteIfBound(Value); });
	Callbacks.OnCancelled = FJWNU_OnSseCancelled::CreateLambda([OnCancelled]() { OnCancelled.ExecuteIfBound(); });
	return UJWNU_GIS_SseClient::CallSseApi_NoTemplate(WorldContextObject, Method, ServiceType, Endpoint, Body, QueryParams, Options, Callbacks, bRequiresAuth);
}
