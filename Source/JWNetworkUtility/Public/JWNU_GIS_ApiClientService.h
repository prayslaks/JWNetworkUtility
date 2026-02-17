// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtility.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNetworkUtilityDelegates.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JsonObjectConverter.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JWNU_GIS_HttpClientHelper.h"
#include "JWNU_GIS_ApiIdentityProvider.h"
#include "JWNU_GIS_ApiHostProvider.h"
#include "JWNU_GIS_ApiClientService.generated.h"

/**
 * 클래스 전용 로그 카테고리 선언
 */
JWNETWORKUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_ApiClientService, Log, All);

/**
 * JW 커스텀 리스폰스 스타일을 가진 API 클라이언트 서비스 클래스.
 */
UCLASS(Config=JWNetworkUtility)
class JWNETWORKUTILITY_API UJWNU_GIS_ApiClientService : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/**
	 * 외부에서 게임 인스턴스 API 서브시스템을 반환하는 함수. (Native CPP)
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @return UJW_GIS_HttpClientHelper 게임인스턴스 서브시스템
	 */
	static UJWNU_GIS_ApiClientService* Get(const UObject* WorldContextObject);
	
	/**
	 * 간편한 API 호출을 지원해주는 함수. 호스트와 인증 토큰은 Config의 설정값에 따라 자동으로 로드된다. 
	 * 401 상태 코드, 즉 토큰 만료 시 자동으로 리프레시하고 재요청하는 내부 플로우를 지원한다.
	 * 템플릿 인자로 전달한 구조체로 리스폰스 바디를 파싱하여 콜백으로 전달한다.
	 * @tparam StructType JSON 리스폰스 바디에서 파싱하길 원하는 언리얼 구조체 타입
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InMethod HTTP 메서드
	 * @param InServiceType 서비스 타입
	 * @param InEndpoint API 엔드포인트
	 * @param InContentBody JSON 바디
	 * @param InQueryParams URL 쿼리 패러미터
	 * @param OnGetCustomStruct 결과 콜백
	 * @param OnHttpRequestJobRetry 재시도 콜백
	 * @param bRequiresAuth 인증 토큰 필요 여부 (false 시 토큰 로직 전체 건너뜀)
	 */
	template<typename StructType>
	static void CallApi_Template(
		const UObject* WorldContextObject,
		const EJWNU_HttpMethod InMethod,
		const EJWNU_ServiceType InServiceType,
		const FString InEndpoint,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		TFunction<void(const StructType&)> OnGetCustomStruct,
		const FOnHttpRequestJobRetryDelegate& OnHttpRequestJobRetry = FOnHttpRequestJobRetryDelegate(),
		const bool bRequiresAuth = true);
	
	/**
	 * 간편한 API 호출을 지원해주는 함수. 호스트와 인증 토큰은 Config의 설정값에 따라 자동으로 로드된다. 
	 * 401 상태 코드, 즉 토큰 만료 시 자동으로 리프레시하고 재요청하는 내부 플로우를 지원한다.
	 * 블루프린트를 위한 비템플릿 버전으로 리스폰스 바디를 그대로 콜백으로 전달한다.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InMethod HTTP 메서드
	 * @param InServiceType 서비스 타입
	 * @param InEndpoint API 엔드포인트
	 * @param InContentBody JSON 바디
	 * @param InQueryParams URL 쿼리 패러미터
	 * @param OnHttpResponse 결과 콜백
	 * @param OnHttpRequestJobRetry 재시도 콜백
	 * @param bRequiresAuth 인증 토큰 필요 여부 (false 시 토큰 로직 전체 건너뜀)
	 */
	static void CallApi_NoTemplate(
		const UObject* WorldContextObject,
		const EJWNU_HttpMethod InMethod,
		const EJWNU_ServiceType InServiceType,
		const FString& InEndpoint,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		const FOnHttpResponseDelegate& OnHttpResponse,
		const FOnHttpRequestJobRetryDelegate& OnHttpRequestJobRetry = FOnHttpRequestJobRetryDelegate(),
		const bool bRequiresAuth = true);
	
private:
	
	/**
	 * 동일한 이름의 정적 함수에 의해 호출되어, 실제로 처리하는 비정적 함수.
	 * @param InMethod REST API HTTP 메서드
	 * @param InServiceType 서비스 타입 (호스트 및 인증 토큰 자동 획득용)
	 * @param InURL REST API를 호출하는 URL
	 * @param InAccessToken 엑세스 인증 토큰
	 * @param InContentBody JSON 바디
	 * @param InQueryParams URL 쿼리 패러미터
	 * @param OnHttpResponse 리스폰스 바디를 전달받는 콜백
	 * @param OnHttpRequestJobRetry 재시도 콜백
	 * @param bTryTokenRefreshing 토큰 리프레시 시도 여부
	 */
	void CallApi_NoTemplate_Execution(
		const EJWNU_HttpMethod InMethod,
		const EJWNU_ServiceType InServiceType,
		const FString& InURL,
		const FString& InAccessToken,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		const FOnHttpResponseDelegate& OnHttpResponse,
		const FOnHttpRequestJobRetryDelegate& OnHttpRequestJobRetry = FOnHttpRequestJobRetryDelegate(),
		const bool bTryTokenRefreshing = true);

	/**
	 * 동일한 이름의 정적 템플릿 함수에 의해 호출되어, 실제로 처리하는 비정적 템플릿 함수.
	 * @tparam StructType JSON 리스폰스 바디에서 파싱하길 원하는 언리얼 구조체 타입
	 * @param InMethod REST API HTTP 메서드
	 * @param InServiceType 서비스 타입 (호스트 및 인증 토큰 자동 획득용)
	 * @param InURL REST API를 호출하는 URL
	 * @param InAccessToken 엑세스 인증 토큰
	 * @param InContentBody JSON 바디
	 * @param InQueryParams URL 쿼리 패러미터
	 * @param OnGetCustomStruct 리스폰스 바디를 파싱한 언리얼 구조체를 전달받는 콜백
	 * @param OnHttpRequestJobRetry 재시도 콜백
	 * @param bTryTokenRefreshing 토큰 리프레시 시도 여부
	 */
	template<typename StructType>
	void CallApi_Template_Execution(
		const EJWNU_HttpMethod InMethod,
		const EJWNU_ServiceType InServiceType,
		const FString& InURL,
		const FString& InAccessToken,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		TFunction<void(const StructType&)> OnGetCustomStruct,
		const FOnHttpRequestJobRetryDelegate& OnHttpRequestJobRetry = FOnHttpRequestJobRetryDelegate(),
		const bool bTryTokenRefreshing = true);

	/**
	 * 잡을 ServiceType별 대기열에 적재하고, 리프레시가 아직 진행 중이 아니라면 ExecuteTokenRefresh를 시작한다.
	 * @param InServiceType 대상 서비스 타입
	 * @param InJob 대기열에 적재할 잡
	 */
	void RequestTokenRefresh(EJWNU_ServiceType InServiceType, FJWNU_PendingJob&& InJob);

	/**
	 * 리프레시 토큰 API를 실제로 1회 호출하는 함수. 완료 시 Drain 함수가 대기열의 잡을 일괄 처리한다.
	 * @param InServiceType 대상 서비스 타입
	 */
	void ExecuteTokenRefresh(EJWNU_ServiceType InServiceType);

	/**
	 * 토큰 리프레시 성공 시, 대기열의 모든 잡에 새 토큰을 전달하고 큐를 비우는 함수.
	 * @param InServiceType 대상 서비스 타입
	 * @param NewAccessToken 새로 발급된 엑세스 토큰
	 */
	void DrainPendingJobs_Success(EJWNU_ServiceType InServiceType, const FString& NewAccessToken);

	/**
	 * 토큰 리프레시 실패 시, 대기열의 모든 잡에 에러를 전달하고 큐를 비우는 함수.
	 * @param InServiceType 대상 서비스 타입
	 * @param ErrorCode 에러 코드
	 * @param ErrorMessage 에러 메시지
	 */
	void DrainPendingJobs_Failure(EJWNU_ServiceType InServiceType, const FString& ErrorCode, const FString& ErrorMessage);

	/**
	 * 리프레시 토큰 API URL을 구축하는 함수.
	 * @return 리프레시 토큰 API의 전체 URL
	 */
	FString BuildRefreshTokenURL() const;

	/**
	 * ServiceType별 토큰 리프레시 진행 중 여부를 나타내는 플래그 맵.
	 */
	TMap<EJWNU_ServiceType, bool> RefreshInProgressFlags;

	/**
	 * ServiceType별 토큰 리프레시 대기열 맵. 리프레시 완료 시 일괄 처리된다.
	 */
	TMap<EJWNU_ServiceType, TArray<FJWNU_PendingJob>> PendingJobQueues;
};

template <typename StructType>
void UJWNU_GIS_ApiClientService::CallApi_Template(
	const UObject* WorldContextObject, 
	const EJWNU_HttpMethod InMethod,
	const EJWNU_ServiceType InServiceType, 
	const FString InEndpoint, 
	const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams, 
	TFunction<void(const StructType&)> OnGetCustomStruct,
	const FOnHttpRequestJobRetryDelegate& OnHttpRequestJobRetry, 
	const bool bRequiresAuth)
{
	// 객체 획득
	UJWNU_GIS_ApiClientService* Self = Get(WorldContextObject);
	if (Self == nullptr)
	{
		return;
	}

	// 호스트 프로바이더에서 호스트 획득
	FString ProvidedHost;
	if (const auto HostProvider = UJWNU_GIS_ApiHostProvider::Get(WorldContextObject))
	{
		if (HostProvider->GetHost(InServiceType, ProvidedHost) == false)
		{
			PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("호스트 획득 실패!"));
			StructType ErrorResult;
			ErrorResult.Code = TEXT("HOST_NOT_FOUND");
			ErrorResult.Message = TEXT("Failed to get host from provider");
			OnGetCustomStruct(ErrorResult);
			return;
		}
	}

	// API URL 구축
	FString ConstructedURL = ProvidedHost + InEndpoint;

	// 인증이 필요하지 않은 경우, 토큰 로직을 건너뛰고 바로 실행
	if (bRequiresAuth == false)
	{
		Self->CallApi_Template_Execution(InMethod, InServiceType, ConstructedURL, TEXT(""), InContentBody, InQueryParams, OnGetCustomStruct, OnHttpRequestJobRetry, false);
		return;
	}

	// 토큰 프로바이더에서 엑세스 토큰 획득
	FJWNU_AccessTokenContainer ProvidedAccessTokenContainer;
	if (const auto TokenProvider = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		if (TokenProvider->GetAccessTokenContainer(InServiceType, ProvidedAccessTokenContainer) == false)
		{
			PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("엑세스 토큰 획득 실패!"));
			StructType ErrorResult;
			ErrorResult.Code = TEXT("TOKEN_NOT_FOUND");
			ErrorResult.Message = TEXT("Failed to get access token from provider");
			OnGetCustomStruct(ErrorResult);
			return;
		}
	}
	else
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("토큰 프로바이더 획득 실패!"));
		StructType ErrorResult;
		ErrorResult.Code = TEXT("PROVIDER_NOT_FOUND");
		ErrorResult.Message = TEXT("Failed to get token provider");
		OnGetCustomStruct(ErrorResult);
		return;
	}
	
	// 토큰 만료 또는 리프레시 진행 중인 경우 잡 큐에 적재
	const int64 CurrentUnixTime = FDateTime::UtcNow().ToUnixTimestamp();
	const bool bTokenExpired = ProvidedAccessTokenContainer.ExpiresAt > 0 && CurrentUnixTime >= (ProvidedAccessTokenContainer.ExpiresAt - 30);
	if (bTokenExpired || Self->RefreshInProgressFlags.FindOrAdd(InServiceType))
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("엑세스 토큰 만료 또는 리프레시 진행 중, 잡 큐에 적재..."));
		JWNU_SCREEN_DEBUG(-1, 5.0f, FColor::Yellow, TEXT("[JWNU] Access Token Expired — Queuing refresh for %s"), *UEnum::GetValueAsString(InServiceType));
		FJWNU_PendingJob Job;
		Job.RequestInfo.ServiceType = InServiceType;
		Job.RequestInfo.Method = InMethod;
		Job.RequestInfo.URL = ConstructedURL;
		Job.RequestInfo.ContentBody = InContentBody;
		Job.RequestInfo.QueryParams = InQueryParams;
		Job.OnTokenReady = [Self, InMethod, InServiceType, ConstructedURL, InContentBody, InQueryParams, OnGetCustomStruct, OnHttpRequestJobRetry](const FString& NewAccessToken)
		{
			Self->CallApi_Template_Execution<StructType>(InMethod, InServiceType, ConstructedURL, NewAccessToken, InContentBody, InQueryParams, OnGetCustomStruct, OnHttpRequestJobRetry, false);
		};
		Job.OnTokenFailed = [OnGetCustomStruct](const FString& ErrorCode, const FString& ErrorMessage)
		{
			StructType ErrorResult;
			ErrorResult.Code = ErrorCode;
			ErrorResult.Message = ErrorMessage;
			OnGetCustomStruct(ErrorResult);
		};
		Self->RequestTokenRefresh(InServiceType, MoveTemp(Job));
		return;
	}

	// 실제 처리
	Self->CallApi_Template_Execution(InMethod, InServiceType, ConstructedURL, ProvidedAccessTokenContainer.AccessToken, InContentBody, InQueryParams, OnGetCustomStruct, OnHttpRequestJobRetry, true);
}

template <typename StructType>
void UJWNU_GIS_ApiClientService::CallApi_Template_Execution(
	const EJWNU_HttpMethod InMethod,
	const EJWNU_ServiceType InServiceType, 
	const FString& InURL, 
	const FString& InAccessToken,
	const FString& InContentBody, 
	const TMap<FString, FString>& InQueryParams,
	TFunction<void(const StructType&)> OnGetCustomStruct, 
	const FOnHttpRequestJobRetryDelegate& OnHttpRequestJobRetry,
	const bool bTryTokenRefreshing)
{
	if (bTryTokenRefreshing)
	{
		// 원래 요청 정보 저장 (401 발생 시 재시도용)
		FJWNU_PendingApiRequest PendingRequest;
		PendingRequest.ServiceType = InServiceType;
		PendingRequest.Method = InMethod;
		PendingRequest.URL = InURL;
		PendingRequest.ContentBody = InContentBody;
		PendingRequest.QueryParams = InQueryParams;		
	
		// 401 상태 코드를 처리할 수 있는 콜백
		const auto CallbackManage401 = FOnHttpRequestCompletedDelegate::CreateWeakLambda(this,
			[this, PendingRequest, OnGetCustomStruct, OnHttpRequestJobRetry](const int32 StatusCode, const FString& ResponseBody)
			{
				if (StatusCode == 401)
				{
					PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("401 토큰 만료 감지, 잡 큐에 적재 후 리프레시 시도..."));
					JWNU_SCREEN_DEBUG(-1, 5.0f, FColor::Orange, TEXT("[JWNU] 401 Unauthorized — Triggering token refresh for %s"), *UEnum::GetValueAsString(PendingRequest.ServiceType));
					FJWNU_PendingJob Job;
					Job.RequestInfo = PendingRequest;
					Job.OnTokenReady = [this, PendingRequest, OnGetCustomStruct, OnHttpRequestJobRetry](const FString& NewAccessToken)
					{
						CallApi_Template_Execution<StructType>(PendingRequest.Method, PendingRequest.ServiceType, PendingRequest.URL, NewAccessToken, PendingRequest.ContentBody, PendingRequest.QueryParams, OnGetCustomStruct, OnHttpRequestJobRetry, false);
					};
					Job.OnTokenFailed = [OnGetCustomStruct](const FString& ErrorCode, const FString& ErrorMessage)
					{
						StructType ErrorResult;
						ErrorResult.Code = ErrorCode;
						ErrorResult.Message = ErrorMessage;
						OnGetCustomStruct(ErrorResult);
					};
					RequestTokenRefresh(PendingRequest.ServiceType, MoveTemp(Job));
					return;
				}

				// 언리얼 구조체 파싱 시도
				StructType ResultData;
				if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseBody, &ResultData, 0, 0))
				{
					OnGetCustomStruct(ResultData);
				}
				else
				{
					ResultData.Code = TEXT("JSON_PARSE_ERROR");
					ResultData.Message = TEXT("Failed to Parse JSON Response Body");
					OnGetCustomStruct(ResultData);
				}
			});
	
		// Http 리퀘스트
		UJWNU_GIS_HttpClientHelper::SendRequest_CustomResponse(GetWorld(), InMethod, InURL, InAccessToken, InContentBody, InQueryParams, CallbackManage401, OnHttpRequestJobRetry);
	}
	else
	{
		// 401 상태 코드를 처리하지 않는 콜백
		const auto CallbackNoManage401 = FOnHttpRequestCompletedDelegate::CreateWeakLambda(this,
			[this, OnGetCustomStruct](const int32 StatusCode, const FString& ResponseBody)
			{
				// 언리얼 구조체 파싱 시도
				StructType ResultData;
				if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseBody, &ResultData, 0, 0))
				{
					OnGetCustomStruct(ResultData);
				}
				else
				{
					ResultData.Code = TEXT("JSON_PARSE_ERROR");
					ResultData.Message = TEXT("Failed to Parse JSON Response Body");
					OnGetCustomStruct(ResultData);
				}	
			});
	
		// Http 리퀘스트
		UJWNU_GIS_HttpClientHelper::SendRequest_CustomResponse(GetWorld(), InMethod, InURL, InAccessToken, InContentBody, InQueryParams, CallbackNoManage401, OnHttpRequestJobRetry);
	}
}
