// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNU_GIS_HttpClientHelper.h"
#include "UObject/Object.h"
#include "JWNU_HttpRequestJob.h"
#include "JWNU_GIS_HttpRequestJobProcessor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_HttpRequestJobProcessor, Log, All);

/**
 * HTTP 요청 Job을 관리하는 서브시스템.
 * 네트워크 레벨 재시도 (5xx, 타임아웃)를 담당한다.
 * 401 토큰 만료 처리는 상위 레이어인 ApiClientService에서 담당.
 */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_HttpRequestJobProcessor : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * HTTP 요청 Job을 생성하고 실행하는 함수.
	 * @param InMethod HTTP 메서드
	 * @param InURL 요청 URL
	 * @param InAuthToken JWT 인증 토큰
	 * @param InContentBody JSON 바디
	 * @param InQueryParams URL 쿼리 패러미터
	 * @param InConfig 요청 설정 (재시도, 타임아웃 등)
	 * @param OnHttpRequestJobCompleted 완료 콜백
	 */
	void ProcessHttpRequestJob(
		const EJWNU_HttpMethod InMethod,
		const FString& InURL,
		const FString& InAuthToken,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		const FJWNU_RequestConfig& InConfig,
		const FOnHttpRequestJobCompletedDelegate& OnHttpRequestJobCompleted);

private:
	/**
	 * 기본 URL에 쿼리 패리미터를 조합해서 최종 URL을 구축하는 함수.
	 * @param BaseURL 리퀘스트를 보낼 기본 URL
	 * @param QueryParams URL 쿼리 패러미터
	 * @return 쿼리 패러미터까지 조합된 최종 URL.
	 */
	static FString BuildURL(const FString& BaseURL, const TMap<FString, FString>& QueryParams);
};
