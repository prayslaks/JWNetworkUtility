// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_HttpRequestJob.h"
#include "Engine/Engine.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWNU_GIS_HttpRequestJobProcessor.generated.h"

/** 클래스 전용의 로그 카테고리 선언 */
JWNETWORKUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_HttpRequestJobProcessor, Log, All);

/** HTTP Job을 생성·설정·실행하는 서브시스템이다. 네트워크 재시도는 Job이, 401 토큰 갱신은 ApiClientService가 담당한다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_HttpRequestJobProcessor : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	
	/** URL 쿼리를 조합하고 설정·콜백을 연결한 HTTP Job을 즉시 실행하는 함수. 반환 객체만으로 실행 성공을 판단하지 않는다. */
	UJWNU_HttpRequestJob* ProcessHttpRequestJob(
		const EJWNU_HttpMethod InMethod,
		const FString& InURL,
		const FString& InAuthToken,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		const FJWNU_RequestConfig& InConfig,
		const FOnHttpRequestJobCompletedDelegate& InOnHttpRequestJobCompleted,
		const FOnHttpRequestJobRetryDelegate& InOnHttpRequestJobRetry = FOnHttpRequestJobRetryDelegate());

private:
	
	/** 쿼리 키·값을 URL 인코딩하고 기존 쿼리 유무에 맞춰 주소에 결합하는 함수. */
	static FString BuildURL(const FString& BaseURL, const TMap<FString, FString>& QueryParams);
	
};
