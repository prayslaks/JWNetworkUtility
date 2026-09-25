// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_GIS_SseClient.h"
#include "JWNU_BFL_SseClient.generated.h"

/** 기존 JSON 변환 노드와 조합하는 BP SSE 진입점이다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_BFL_SseClient : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 직접 URL과 Bearer 토큰으로 스트림을 요청하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU|SSE", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="QueryParams,Options,OnOpened,OnEvent,OnCompleted,OnError,OnCancelled"))
	static UJWNU_HttpRequestJobHandle* SendSseRequest(const UObject* WorldContextObject, EJWNU_HttpMethod Method,
		const FString& Body, const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options,
		const FJWNU_OnSseResponseBP& OnOpened, const FJWNU_OnSseEventBP& OnEvent,
		const FJWNU_OnSseResponseBP& OnCompleted, const FJWNU_OnSseResponseBP& OnError,
		const FJWNU_OnSseCancelledBP& OnCancelled, const FString& URL, const FString& AuthToken);

	/** 서비스 호스트와 JWT 갱신으로 스트림을 요청하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU|SSE", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="QueryParams,Options,OnOpened,OnEvent,OnCompleted,OnError,OnCancelled"))
	static UJWNU_HttpRequestJobHandle* CallSseApi(const UObject* WorldContextObject, EJWNU_HttpMethod Method,
		const FString& Body, const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options,
		const FJWNU_OnSseResponseBP& OnOpened, const FJWNU_OnSseEventBP& OnEvent,
		const FJWNU_OnSseResponseBP& OnCompleted, const FJWNU_OnSseResponseBP& OnError,
		const FJWNU_OnSseCancelledBP& OnCancelled, EJWNU_ServiceType ServiceType, const FString& Endpoint, bool bRequiresAuth = true);
};
