// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_Actor_ApiTest.h"

#include "JWNU_GIS_ApiClientService.h"
#include "JWNU_GIS_ApiIdentityProvider.h"
#include "JWNU_GIS_ApiHostProvider.h"
#include "TimerManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogJWNU_Actor_ApiTest);

AJWNU_Actor_ApiTest::AJWNU_Actor_ApiTest()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AJWNU_Actor_ApiTest::BeginPlay()
{
	Super::BeginPlay();
	
	FTimerDelegate TimerDelegate1;
	TimerDelegate1.BindLambda([this]()
	{
		// 첫 번째 테스트 실행
		TestOne_HttpClientHelper();
	});
	GetWorldTimerManager().SetTimer(TimerHandle1, TimerDelegate1, 1.0f, false);
	
	FTimerDelegate TimerDelegate2;
	TimerDelegate2.BindLambda([this]()
	{
		// 두 번째 테스트 실행
		TestTwo_ApiClientService_Template();
	});
	GetWorldTimerManager().SetTimer(TimerHandle2, TimerDelegate2, 2.0f, false);
	
	FTimerDelegate TimerDelegate3;
	TimerDelegate3.BindLambda([this]()
	{
		// 세 번째 테스트 실행
		TestThree_ApiClientService_NoTemplate();
	});
	GetWorldTimerManager().SetTimer(TimerHandle3, TimerDelegate3, 3.0f, false);
}

void AJWNU_Actor_ApiTest::TestOne_HttpClientHelper() const
{
	PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("==================== 첫 번째 테스트 시작 : Only HTTPClientHelper ===================="));
	
	// RawResponseBody를 가정한 콜백
	const auto Callback = FOnHttpRequestCompletedDelegate::CreateLambda([](const int32 StatusCode, const FString& ResponseBody)
	{
		PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("서버로부터 획득한 리스폰스 바디 : %s"), *ResponseBody);	
		PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("==================== 첫 번째 테스트 종료 : Only HTTPClientHelper ===================="));
	});
		
	// 호스트 획득
	FString ProvidedHost;
	if (UJWNU_GIS_ApiHostProvider::Get(GetWorld())->GetHost(EJWNU_ServiceType::AuthServer, ProvidedHost) == false)
	{
		PRINT_LOG(LogJWNU_Actor_ApiTest, Warning, TEXT("호스트 획득 실패!"));
		return;
	}
	
	// 호스트와 엔드포인트를 결합한 URL
	const FString URL = ProvidedHost + TEXT("/health");
	
	// 엑세스 토큰
	const FString AuthToken = {};
	
	// 콘텐츠 바디
	const FString ContentBody = {};
	
	// 쿼리 패러미터
	const TMap<FString, FString> QueryParams = {};
	
	// HTTP 리퀘스트
	UJWNU_GIS_HttpClientHelper::SendRequest_RawResponse(GetWorld(), EJWNU_HttpMethod::Get, URL, AuthToken, ContentBody, QueryParams, Callback);
}

void AJWNU_Actor_ApiTest::TestTwo_ApiClientService_Template() const
{
	PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("==================== 두 번째 테스트 시작 : ApiClientService Template ===================="));
	
	// JSON 리스폰스 바디가 자동으로 언리얼 구조체로 파싱되어 돌아오는 콜백
	const auto Callback = [](const FJWNU_RES_Base& Response)
	{
		PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("서버로부터 획득한 리스폰스 객체 : %s %s"), *Response.Code, *Response.Message);
		PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("==================== 두 번째 테스트 종료 : ApiClientService Template ===================="));
	};
	
	// 엔드포인트
	const FString Endpoint = TEXT("/health");
	
	// 콘텐츠 바디
	const FString ContentBody = {};
	
	// 쿼리 패러미터
	const TMap<FString, FString> QueryParams = {};
	
	// 강력한 구조체 자동화 파싱과 토큰 리프레싱을 지원하는 CPP용 API 함수 호출
	UJWNU_GIS_ApiClientService::CallApi_Template<FJWNU_RES_Base>(GetWorld(), EJWNU_HttpMethod::Get, EJWNU_ServiceType::AuthServer, Endpoint, ContentBody, QueryParams, Callback);
}

void AJWNU_Actor_ApiTest::TestThree_ApiClientService_NoTemplate() const
{
	PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("==================== 세 번째 테스트 시작 : ApiClientService No Template ===================="));
	
	// JSON 리스폰스 바디 그대로 돌아오는 콜백
	const auto Callback = FOnHttpResponseDelegate::CreateLambda([](const EJWNU_HttpStatusCode StatusCode, const FString& ResponseBody)
	{
		PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("서버로부터 획득한 리스폰스 바디 : %s"), *ResponseBody);
		PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("==================== 세 번째 테스트 종료 : ApiClientService No Template ===================="));
	});
	
	// 엔드포인트
	const FString Endpoint = TEXT("/health");
	
	// 콘텐츠 바디
	const FString ContentBody = {};
	
	// 쿼리 패러미터
	const TMap<FString, FString> QueryParams = {};
	
	// 토큰 리프레싱을 지원하는 블루프린트 용 API 함수 호출
	UJWNU_GIS_ApiClientService::CallApi_NoTemplate(GetWorld(), EJWNU_HttpMethod::Get, EJWNU_ServiceType::AuthServer, Endpoint, ContentBody, QueryParams, Callback);
}
