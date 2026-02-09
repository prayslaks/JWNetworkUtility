// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_ApiTest.h"

#include "JWNU_GIS_ApiClientService.h"
#include "JWNU_GIS_ApiTokenProvider.h"
#include "JWNU_GIS_ApiHostProvider.h"

DEFINE_LOG_CATEGORY(LogJWNU_ApiTest);

AJWNU_ApiTest::AJWNU_ApiTest()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AJWNU_ApiTest::BeginPlay()
{
	Super::BeginPlay();
	
	// 첫 번째 테스트 실행
	TestHttpClientHelper();
	
	// 두 번째 테스트 실행
	TestApiClientService();
}

void AJWNU_ApiTest::TestHttpClientHelper() const
{
	// RawResponseBody를 가정한 콜백
	const auto Callback = FOnHttpResponseDelegate::CreateLambda([](const FString& Body)
	{
		PRINT_LOG(LogJWNU_ApiTest, Display, TEXT("서버로부터 획득한 리스폰스 바디 : %s"), *Body);	
	});
		
	// 호스트 획득
	FString ProvidedHost;
	if (UJWNU_GIS_ApiHostProvider::Get(GetWorld())->GetHost(EJWNU_ServiceType::GameServer, ProvidedHost))
	{
		return;	
	}
		
	// 엑세스 토큰 획득
	FString ProvidedToken;
	if (UJWNU_GIS_ApiTokenProvider::Get(GetWorld())->GetAccessToken(EJWNU_ServiceType::GameServer, ProvidedToken))
	{
		return;
	}
	
	// 호스트와 엔드포인트를 결합한 URL
	const FString URL = ProvidedHost + TEXT("/health/login");
	
	// 콘텐츠 바디
	const FString ContentBody = {};
	
	// 쿼리 패러미터
	const TMap<FString, FString> QueryParams = {};
	
	// HTTP 리퀘스트
	UJWNU_GIS_HttpClientHelper::SendReqeust_RawResponse(GetWorld(), EJWNU_HttpMethod::GET, URL, ProvidedToken, ContentBody, QueryParams, Callback);
}

void AJWNU_ApiTest::TestApiClientService() const
{
	// 언리얼 구조체로 돌아오는 콜백
	const auto Callback = [](const FJWNU_BaseResponseData& Response)
	{
		PRINT_LOG(LogJWNU_ApiTest, Display, TEXT("서버로부터 획득한 리스폰스 객체 : %s %s"), *Response.Code, *Response.Message);
	};
	
	// 호스트와 엔드포인트를 결합한 URL
	const FString Endpoint = TEXT("/health/login");
	
	// 콘텐츠 바디
	const FString ContentBody = {};
	
	// 쿼리 패러미터
	const TMap<FString, FString> QueryParams = {};
	
	// REST API 호출
	UJWNU_GIS_ApiClientService::CallApi<FJWNU_BaseResponseData>(GetWorld(), EJWNU_HttpMethod::GET, EJWNU_ServiceType::GameServer, Endpoint, ContentBody, QueryParams, Callback);
}
