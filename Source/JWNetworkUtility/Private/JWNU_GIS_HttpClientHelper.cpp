// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_HttpClientHelper.h"

#include "JWNetworkUtility.h"
#include "JWNU_GIS_HttpRequestJobProcessor.h"
#include "JWNU_HttpRequestJob.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogJWNU_GIS_HttpClientHelper);

void UJWNU_GIS_HttpClientHelper::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	StatusCodeToCustomCodeMap.Emplace(400, TEXT("BAD_REQUEST"));
	StatusCodeToCustomCodeMap.Emplace(401, TEXT("UNAUTHORIZED"));
	StatusCodeToCustomCodeMap.Emplace(402, TEXT("PAYMENT_REQUIRED"));
	StatusCodeToCustomCodeMap.Emplace(403, TEXT("FORBIDDEN"));
	StatusCodeToCustomCodeMap.Emplace(404, TEXT("NOT_FOUND"));
	StatusCodeToCustomCodeMap.Emplace(405, TEXT("METHOD_NOT_ALLOWED"));
	StatusCodeToCustomCodeMap.Emplace(406, TEXT("NOT_ACCEPTABLE"));
	StatusCodeToCustomCodeMap.Emplace(407, TEXT("PROXY_AUTH_REQUIRED"));
	StatusCodeToCustomCodeMap.Emplace(408, TEXT("REQUEST_TIMEOUT"));
	StatusCodeToCustomCodeMap.Emplace(500, TEXT("INTERNAL_SERVER_ERROR"));
	StatusCodeToCustomCodeMap.Emplace(501, TEXT("NOT_IMPLEMENTED"));
	StatusCodeToCustomCodeMap.Emplace(502, TEXT("BAD_GATEWAY"));
	StatusCodeToCustomCodeMap.Emplace(503, TEXT("SERVICE_UNAVAILABLE"));
	StatusCodeToCustomCodeMap.Emplace(504, TEXT("GATEWAY_TIMEOUT"));
	
	StatusCodeToCustomMessageMap.Emplace(400, TEXT("Bad Request"));
	StatusCodeToCustomMessageMap.Emplace(401, TEXT("Unauthorized"));
	StatusCodeToCustomMessageMap.Emplace(402, TEXT("Payment Required"));
	StatusCodeToCustomMessageMap.Emplace(403, TEXT("Forbidden"));
	StatusCodeToCustomMessageMap.Emplace(404, TEXT("Not Found"));
	StatusCodeToCustomMessageMap.Emplace(405, TEXT("Method Not Allowed"));
	StatusCodeToCustomMessageMap.Emplace(406, TEXT("Not Acceptable"));
	StatusCodeToCustomMessageMap.Emplace(407, TEXT("Proxy Authentication Required"));
	StatusCodeToCustomMessageMap.Emplace(408, TEXT("Request Timeout"));
	StatusCodeToCustomMessageMap.Emplace(500, TEXT("Internal Server Error"));
	StatusCodeToCustomMessageMap.Emplace(501, TEXT("Not Implemented"));
	StatusCodeToCustomMessageMap.Emplace(502, TEXT("Bad Gateway"));
	StatusCodeToCustomMessageMap.Emplace(503, TEXT("Service Unavailable"));
	StatusCodeToCustomMessageMap.Emplace(504, TEXT("Gateway Timeout"));
}

UJWNU_GIS_HttpClientHelper* UJWNU_GIS_HttpClientHelper::Get(const UObject* WorldContextObject)
{
	// 월드 컨텍스트 오브젝트 이상
	if (WorldContextObject == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_HttpClientHelper, Warning, TEXT("월드 컨텍스트 오브젝트 이상!"));
		return nullptr;
	}

	// 월드 획득
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_HttpClientHelper, Warning, TEXT("월드 획득 실패!"));
		return nullptr;
	}

	// 게임인스턴스 획득
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_HttpClientHelper, Warning, TEXT("게임인스턴스 획득 실패!"));
		return nullptr;
	}
    
	// HttpClientHelper 게임인스턴스 서브시스템 반환
	return GameInstance->GetSubsystem<UJWNU_GIS_HttpClientHelper>();
}

void UJWNU_GIS_HttpClientHelper::SendRequest_RawResponse(
	const UObject* WorldContextObject,
	const EJWNU_HttpMethod InMethod, 
	const FString& InURL, 
	const FString& InAuthToken, 
	const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams, 
	const FOnHttpRequestCompletedDelegate& InOnHttpResponse,
	const FOnHttpRequestJobRetryDelegate& InOnHttpRequestJobRetry)
{
	// 객체 획득
	UJWNU_GIS_HttpClientHelper* Self = Get(WorldContextObject);
	if (Self == nullptr)
	{
		return;
	}
	
	// 실제 처리
	Self->SendRequest_RawResponse(InMethod, InURL, InAuthToken, InContentBody, InQueryParams, InOnHttpResponse, InOnHttpRequestJobRetry);
}

void UJWNU_GIS_HttpClientHelper::SendRequest_CustomResponse(
	const UObject* WorldContextObject,
	const EJWNU_HttpMethod InMethod, 
	const FString& InURL, 
	const FString& InAuthToken, 
	const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams, 
	const FOnHttpRequestCompletedDelegate& InOnHttpResponse,
	const FOnHttpRequestJobRetryDelegate& InOnHttpRequestJobRetry)
{
	// 객체 획득
	UJWNU_GIS_HttpClientHelper* Self = Get(WorldContextObject);
	if (Self == nullptr)
	{
		return;
	}
	
	Self->SendRequest_CustomResponse(InMethod, InURL, InAuthToken, InContentBody, InQueryParams, InOnHttpResponse, InOnHttpRequestJobRetry);
}

void UJWNU_GIS_HttpClientHelper::SendRequest_RawResponse(
	const EJWNU_HttpMethod InMethod, 
	const FString& InURL,
	const FString& InAuthToken, 
	const FString& InContentBody, 
	const TMap<FString, FString>& InQueryParams,
	const FOnHttpRequestCompletedDelegate& InOnHttpResponse,
	const FOnHttpRequestJobRetryDelegate& InOnHttpRequestJobRetry)
{
	// 서브시스템 획득
	UJWNU_GIS_HttpRequestJobProcessor* Subsystem = GetGameInstance()->GetSubsystem<UJWNU_GIS_HttpRequestJobProcessor>();
	if (Subsystem == nullptr)
	{
		return;
	}
	
	// 콜백에서 리스폰스를 외부 델리게이트에 전달하게 된다
	FOnHttpRequestJobCompletedDelegate Callback;
	Callback.BindWeakLambda(this, [this, InOnHttpResponse](const bool bNetworkAvailable, const int32 StatusCode, const FString& ResponseBody)
		{
			const FString Result = (bNetworkAvailable ? TEXT("Success") : TEXT("Fail"));
			PRINT_LOG(LogJWNU_GIS_HttpClientHelper, Display, TEXT("%d, %s"), StatusCode, *Result);
			InOnHttpResponse.ExecuteIfBound(StatusCode, ResponseBody);
		});
	
	// 콜백과 패러미터를 잡 프로세서에 넘긴다
	Subsystem->ProcessHttpRequestJob(InMethod, InURL, InAuthToken, InContentBody, InQueryParams, DefaultRequestConfig, Callback, InOnHttpRequestJobRetry);
}

void UJWNU_GIS_HttpClientHelper::SendRequest_CustomResponse(
	const EJWNU_HttpMethod InMethod, 
	const FString& InURL,
	const FString& InAuthToken, 
	const FString& InContentBody, 
	const TMap<FString, FString>& InQueryParams,
	const FOnHttpRequestCompletedDelegate& InOnHttpResponse,
	const FOnHttpRequestJobRetryDelegate& InOnHttpRequestJobRetry)
{
		// 서브시스템 획득
	UJWNU_GIS_HttpRequestJobProcessor* Subsystem = GetGameInstance()->GetSubsystem<UJWNU_GIS_HttpRequestJobProcessor>();
	if (Subsystem == nullptr)
	{
		return;
	}
	
	// 콜백에서 리스폰스를 전처리해서 외부 델리게이트에 전달하게 된다
	FOnHttpRequestJobCompletedDelegate Callback;
	Callback.BindWeakLambda(this, [this, InOnHttpResponse](const bool bNetworkAvailable, const int32 StatusCode, const FString& ResponseBody)
	{
		const FString Result = (bNetworkAvailable ? TEXT("Success") : TEXT("Fail"));
		PRINT_LOG(LogJWNU_GIS_HttpClientHelper, Display, TEXT("%d, %s"), StatusCode, *Result);
	
		// 1. 네트워크 연결 실패 시
		if (bNetworkAvailable == false)
		{
			// { success = false, code = FinalCode, message = FinalMessage } 구조의 가짜 JSON 리스폰스 바디 생성
			const FString FakeResponseBody = TEXT("{\"success\": false, \"code\": \"NETWORK_ERROR\", \"message\": \"Failed to Send HTTP Request\"}");
			InOnHttpResponse.ExecuteIfBound(StatusCode, FakeResponseBody);
			return;
		}
				
		// 2. HTTP 상태 코드에 따른 분기
		if (StatusCode >= 200 && StatusCode < 300)
		{
			// 서버 로직에 도달했을 경우 진짜 JSON 리스폰스 바디를 콜백으로 전달
			InOnHttpResponse.ExecuteIfBound(StatusCode, ResponseBody);
		}
		else
		{
			// 커스텀 코드 획득
			FString FakeCustomCode = TEXT("UNKNOWN_ERROR");
			if (const FString* FoundCode = StatusCodeToCustomCodeMap.Find(StatusCode))
			{
				FakeCustomCode = *FoundCode;
			}
					
			// 커스텀 메시지 획득
			FString FakeCustomMessage = TEXT("Unknown Error");
			if (const FString* FoundMessage = StatusCodeToCustomMessageMap.Find(StatusCode))
			{
				FakeCustomMessage = *FoundMessage;
			}
					
			// { success = false, code = FinalCode, message = FinalMessage } 구조의 가짜 JSON 리스폰스 바디 생성
			const FString FakeResponseBody = FString::Printf(
			TEXT("{\"success\": false, \"code\": \"%s\", \"message\": \"%s\"}"), 
				*FakeCustomCode, 
				*FakeCustomMessage);
				
			// 상태 코드에 따른 가짜 JSON 리스폰스 바디를 콜백으로 전달
			InOnHttpResponse.ExecuteIfBound(StatusCode, FakeResponseBody);
		}
	});
	
	// 콜백과 패러미터를 잡 프로세서에 넘긴다
	Subsystem->ProcessHttpRequestJob(InMethod, InURL, InAuthToken, InContentBody, InQueryParams, DefaultRequestConfig, Callback, InOnHttpRequestJobRetry);
}
