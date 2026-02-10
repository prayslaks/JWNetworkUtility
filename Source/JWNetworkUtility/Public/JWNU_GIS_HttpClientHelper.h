// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNetworkUtilityDelegates.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWNU_GIS_HttpClientHelper.generated.h"

/**
 * 클래스 전용 로그 카테고리 선언
 */
JWNETWORKUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_HttpClientHelper, Log, All);

/**
 * HTTP 클라이언트 헬퍼 게임인스턴스 서브시스템 클래스.
 * 패러미터만 전달해도 적절한 HTTP 리퀘스트 객체를 생성해서 대신 전송하고, 리스폰스를 콜백으로 돌려준다.
 */
UCLASS(Config=JWNetworkUtility)
class JWNETWORKUTILITY_API UJWNU_GIS_HttpClientHelper : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	/**
	 * 외부에서 HTTP 클라이언트 헬퍼 서브시스템을 획득하기 위해 호출하는 함수. (네이티브 C++ 용)
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @return UJW_GIS_HttpClientHelper HTTP 클라이언트 헬퍼 서브시스템
	 */
	static UJWNU_GIS_HttpClientHelper* Get(const UObject* WorldContextObject);
	
	/**
	 * HTTP 리퀘스트를 보내는 함수. 전처리하지 않은 Raw Response Body를 콜백으로 반환한다.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InMethod HTTP 메서드
	 * @param InURL 리퀘스트를 보낼 URL
	 * @param InAuthToken 헤더에 탑재할 JWT 인증 토큰
	 * @param InContentBody JSON 등으로 이루어진 콘텐츠 바디
	 * @param InQueryParams  URL에 부착하는 쿼리 패리미터
	 * @param InOnHttpResponse 서버 로직 도달 여부와 리스폰스 바디를 전달하는 콜백 델리게이트
	 */
	static void SendReqeust_RawResponse(
		const UObject* WorldContextObject,
		const EJWNU_HttpMethod InMethod, 
		const FString& InURL, 
		const FString& InAuthToken, 
		const FString& InContentBody, 
		const TMap<FString, FString>& InQueryParams, 
		const FOnHttpRequestCompletedDelegate& InOnHttpResponse);
	
	/**
	 * HTTP 리퀘스트를 보내는 함수. 전처리된 Custom Response Body를 콜백으로 반환한다.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InMethod HTTP 메서드
	 * @param InURL 리퀘스트를 보낼 URL
	 * @param InAuthToken 헤더에 탑재할 JWT 인증 토큰
	 * @param InContentBody JSON 등으로 이루어진 콘텐츠 바디
	 * @param InQueryParams  URL에 부착하는 쿼리 패리미터
	 * @param InOnHttpResponse 서버 로직 도달 여부와 리스폰스 바디를 전달하는 콜백 델리게이트
	 */
	static void SendReqeust_CustomResponse(
		const UObject* WorldContextObject,
		const EJWNU_HttpMethod InMethod, 
		const FString& InURL, 
		const FString& InAuthToken, 
		const FString& InContentBody, 
		const TMap<FString, FString>& InQueryParams, 
		const FOnHttpRequestCompletedDelegate& InOnHttpResponse);
	
private:
	
	/**
	 * 동일한 이름의 정적 함수에 의해 호출되어, 실제로 처리하는 비정적 함수.
	 * @param InMethod HTTP 메서드
	 * @param InURL 리퀘스트를 보낼 URL
	 * @param InAuthToken 헤더에 탑재할 JWT 인증 토큰
	 * @param InContentBody JSON 등으로 이루어진 콘텐츠 바디
	 * @param InQueryParams  URL에 부착하는 쿼리 패리미터
	 * @param InOnHttpResponse 서버 로직 도달 여부와 리스폰스 바디를 전달하는 콜백 델리게이트
	 */
	void SendReqeust_RawResponse(
		const EJWNU_HttpMethod InMethod, 
		const FString& InURL, 
		const FString& InAuthToken, 
		const FString& InContentBody, 
		const TMap<FString, FString>& InQueryParams, 
		const FOnHttpRequestCompletedDelegate& InOnHttpResponse);
	
	/**
	 * 동일한 이름의 정적 함수에 의해 호출되어, 실제로 처리하는 비정적 함수.
	 * @param InMethod HTTP 메서드
	 * @param InURL 리퀘스트를 보낼 URL
	 * @param InAuthToken 헤더에 탑재할 JWT 인증 토큰
	 * @param InContentBody JSON 등으로 이루어진 콘텐츠 바디
	 * @param InQueryParams  URL에 부착하는 쿼리 패리미터
	 * @param InOnHttpResponse 서버 로직 도달 여부와 리스폰스 바디를 전달하는 콜백 델리게이트
	 */
	void SendReqeust_CustomResponse(
		const EJWNU_HttpMethod InMethod, 
		const FString& InURL, 
		const FString& InAuthToken, 
		const FString& InContentBody, 
		const TMap<FString, FString>& InQueryParams, 
		const FOnHttpRequestCompletedDelegate& InOnHttpResponse);
	
	/**
	 * 300번대 이상의 상태 코드를 커스텀 코드로 매핑하는 맵.
	 */
	UPROPERTY()
	TMap<int32, FString> StatusCodeToCustomCodeMap;

	/**
	 *  300번대 이상의 상태 코드를 토대로 커스텀 메시지로 매핑하는 맵.
	 */
	UPROPERTY()
	TMap<int32, FString> StatusCodeToCustomMessageMap;
	
	/**
	 * 기본 HTTP 리퀘스트 설정값을 담은 구조체 필드.
	 */
	UPROPERTY(Config)
	FJWNU_RequestConfig DefaultRequestConfig;
};
