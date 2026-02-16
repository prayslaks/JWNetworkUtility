// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_HttpRequestJobProcessor.h"
#include "JWNetworkUtility.h"
#include "JWNU_HttpRequestJob.h"
#include "GenericPlatform/GenericPlatformHttp.h"

DEFINE_LOG_CATEGORY(LogJWNU_GIS_HttpRequestJobProcessor);

void UJWNU_GIS_HttpRequestJobProcessor::ProcessHttpRequestJob(
	const EJWNU_HttpMethod InMethod,
	const FString& InURL,
	const FString& InAuthToken,
	const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams,
	const FJWNU_RequestConfig& InConfig,
	const FOnHttpRequestJobCompletedDelegate& InOnHttpRequestJobCompleted,
	const FOnHttpRequestJobRetryDelegate& InOnHttpRequestJobRetry)
{
	// 리퀘스트 잡 생성
	UJWNU_HttpRequestJob* RequestJob = NewObject<UJWNU_HttpRequestJob>(this);

	// GET 메서드에 바디가 달려오는 상황은 표준에서 벗어나있다
	if (InMethod == EJWNU_HttpMethod::Get && !InContentBody.IsEmpty())
	{
		PRINT_LOG(LogJWNU_GIS_HttpRequestJobProcessor, Warning, TEXT("HTTP 표준 위반, GET 메서드에 Body를 넣으려는 시도..."))
	}

	// 쿼리 패리미터 합성
	const FString FinalURL = BuildURL(InURL, InQueryParams);

	// 리퀘스트 잡 초기화 및 콜백 바인딩
	RequestJob->Initialize(InMethod, FinalURL, InAuthToken, InContentBody, InConfig);
	RequestJob->OnHttpRequestJobComplete = InOnHttpRequestJobCompleted;
	RequestJob->OnHttpRequestJobRetry = InOnHttpRequestJobRetry;

	// 리퀘스트 잡 실행 및 확인
	if (RequestJob->Execute())
	{
		PRINT_LOG(LogJWNU_GIS_HttpRequestJobProcessor, Display, TEXT("%s 실행 정상!"), *InURL);
	}
	else
	{
		PRINT_LOG(LogJWNU_GIS_HttpRequestJobProcessor, Warning, TEXT("%s 실행 이상!"), *InURL);
	}
}

FString UJWNU_GIS_HttpRequestJobProcessor::BuildURL(const FString& BaseURL, const TMap<FString, FString>& QueryParams)
{
	if (QueryParams.Num() == 0)
	{
		return BaseURL;
	}

	FString CombinedParams = "";
	for (auto& It : QueryParams)
	{
		// 특수 문자가 포함될 수 있으므로 인코딩 처리
		FString EncodedKey = FGenericPlatformHttp::UrlEncode(It.Key);
		FString EncodedValue = FGenericPlatformHttp::UrlEncode(It.Value);
		CombinedParams += FString::Printf(TEXT("%s=%s&"), *EncodedKey, *EncodedValue);
	}

	// 마지막에 붙은 '&' 제거
	CombinedParams.LeftChopInline(1);

	// 기존 URL에 ?가 포함되어 있는지 확인 후 결합
	const FString Separator = BaseURL.Contains(TEXT("?")) ? TEXT("&") : TEXT("?");

	// 반환
	return FString::Printf(TEXT("%s%s%s"), *BaseURL, *Separator, *CombinedParams);
}
