// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_SseTypes.h"
#include "JWNU_BFL_SseClient.generated.h"

class UJWNU_SseApiRequest;

DECLARE_DYNAMIC_DELEGATE_OneParam(FJWNU_OnSseResponseBP, const FJWNU_SseResponse&, Response);
DECLARE_DYNAMIC_DELEGATE_OneParam(FJWNU_OnSseEventBP, const FJWNU_SseEvent&, Event);
DECLARE_DYNAMIC_DELEGATE(FJWNU_OnSseCancelledBP);

/** 응답 콜백을 입력받아 SSE를 즉시 실행하는 편의 함수 라이브러리다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_BFL_SseClient : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 서비스 Host·JWT로 즉시 시작하는 함수. 반환 요청은 선택적 취소·조회용이며 Start를 다시 호출하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|SSE", meta=(WorldContext="WorldContextObject", DisplayName="Call SSE API", AutoCreateRefTerm="QueryParams,Options,OnOpened,OnEvent,OnCompleted,OnError,OnCancelled"))
	static UJWNU_SseApiRequest* CallSseApi(const UObject* WorldContextObject, EJWNU_HttpMethod Method,
		const FString& Body, const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options,
		const FJWNU_OnSseResponseBP& OnOpened, const FJWNU_OnSseEventBP& OnEvent,
		const FJWNU_OnSseResponseBP& OnCompleted, const FJWNU_OnSseResponseBP& OnError,
		const FJWNU_OnSseCancelledBP& OnCancelled, EJWNU_ServiceType ServiceType, const FString& Endpoint, bool bRequiresAuth = true);
};
