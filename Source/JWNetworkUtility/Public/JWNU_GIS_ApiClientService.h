// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JsonObjectConverter.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "JWNetworkUtility.h"
#include "JWNU_GIS_HttpClientHelper.h"
#include "JWNU_GIS_ApiTokenProvider.h"
#include "JWNU_GIS_ApiHostProvider.h"
#include "JWNU_GIS_ApiClientService.generated.h"

/**
 * 클래스 전용 로그 카테고리 선언
 */
DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_ApiClientService, Log, All);

/**
 * API 콜백 템플릿 핸들러.
 * 1. 외부에서 API를 호출할 때 파싱되길 원하는 구조체를 템플릿으로 가지는 델리게이트를 핸들러에 넘긴다
 * 2. 핸들러는 HTTP 리스폰스가 돌아오면 JSON 리스폰스 바디 파싱을 시도하고, 그 결과를 구조체에 담는다
 * 3. 파싱에 실패하면 CustomCode에 JSON_PARSE_ERROR가, 성공하면 그 이외의 값이 있다 
 * @tparam StructType JSON 리스폰스 바디에서 파싱하길 원하는 언리얼 구조체 타입
 * @param OnResult 해당 언리얼 구조체 타입으로 결과를 돌려받는 콜백 델리게이트
 * @return JW_GIS_HttpClientHelper에게 넘겨서 리스폰스 바디를 전달받기 위한 콜백 델리게이트 
 */
template<typename StructType>
static TDelegate<void(const FString&)> HandleResponse(TFunction<void(const StructType&)> OnResult)
{
	return FOnHttpResponseDelegate::CreateLambda([OnResult](const FString& ResponseBody)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("%s"), *ResponseBody);
		
		// 지정한 구조체 타입으로 변환 시도
		StructType ResultData;
		if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseBody, &ResultData, 0, 0))
		{
			OnResult(ResultData);
		}
		else
		{
			ResultData.Code = TEXT("JSON_PARSE_ERROR");
			ResultData.Message = TEXT("Failed to Parse JSON Response Body");
			OnResult(ResultData);
		}
	});
}

/**
 * JW 커스텀 스타일 서버의 기본 리스폰스 스키마를 가진 언리얼 구조체.
 */
USTRUCT(BlueprintType)
struct FJWNU_BaseResponseData
{
	GENERATED_BODY()

	/**
	 * 비즈니스 로직의 성공 여부를 나타내는 불 필드. 
	 */
	UPROPERTY(BlueprintReadOnly, Category="API Response")
	bool Success;

	/**
	 * 네트워크, 파싱, 비즈니스 상태를 나타내는 커스텀 코드 문자열 필드.
	 */
	UPROPERTY(BlueprintReadOnly, Category="API Response")
	FString Code;

	/**
	 * 네트워크, 파싱, 비즈니스 결과를 나타내는 메시지 문자열 필드.
	 */
	UPROPERTY(BlueprintReadOnly, Category="API Response")
	FString Message;

	/**
	 * 기본 생성자
	 */
	FJWNU_BaseResponseData()
	{
		Success = false;
		Code = TEXT("UNKNOWN_ERROR");
		Message = TEXT("There is an Unknown Error");
	}
};

/**
 * 토큰 리프레시 API 응답 구조체.
 */
USTRUCT(BlueprintType)
struct FJWNU_RefreshTokenResponseData : public FJWNU_BaseResponseData
{
	GENERATED_BODY()

	/**
	 * 새로 발급된 엑세스 토큰.
	 */
	UPROPERTY(BlueprintReadOnly, Category="API Response")
	FString AccessToken;

	/**
	 * 새로 발급된 리프레시 토큰
	 */
	UPROPERTY(blueprintReadOnly, Category="API Response")
	FString RefreshToken;

	/**
	 * 기본 생성자
	 */
	FJWNU_RefreshTokenResponseData()
	{
		AccessToken = TEXT("");
		RefreshToken = TEXT("");
	}
};

/**
 * 401 발생 시 원래 요청을 재시도하기 위한 정보를 담는 구조체.
 */
USTRUCT()
struct FJWNU_PendingApiRequest
{
	GENERATED_BODY()

	EJWNU_ServiceType ServiceType;
	EJWNU_HttpMethod Method;
	FString URL;
	FString ContentBody;
	TMap<FString, FString> QueryParams;
};

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
	 * API를 호출하는 함수. 호스트와 인증 토큰은 Config의 설정값에 따라 자동으로 로드된다. 401 토큰 만료 시 자동으로 리프레시하고 재요청하는 내부 플로우 지원.
	 * @tparam StructType JSON 리스폰스 바디에서 파싱하길 원하는 언리얼 구조체 타입
	 * @param WorldContextObject 
	 * @param InMethod HTTP 메서드
	 * @param InServiceType 서비스 타입 (호스트 및 인증 토큰 자동 획득용)
	 * @param InEndpoint API 엔드포인트
	 * @param InContentBody JSON 바디
	 * @param InQueryParams URL 쿼리 패러미터
	 * @param OnGetCustomStruct 결과 콜백
	 */
	template<typename StructType>
	static void CallApi(
		const UObject* WorldContextObject,
		const EJWNU_HttpMethod InMethod,
		const EJWNU_ServiceType InServiceType,
		const FString InEndpoint,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		TFunction<void(const StructType&)> OnGetCustomStruct);

private:
	
	/**
	 * 동일한 이름의 정적 함수에 의해 호출되어, 실제로 처리하는 비정적 함수.
	 * @tparam StructType JSON 리스폰스 바디에서 파싱하길 원하는 언리얼 구조체 타입
	 * @param InMethod REST API HTTP 메서드
	 * @param InServiceType 서비스 타입 (호스트 및 인증 토큰 자동 획득용)
	 * @param InURL REST API를 호출하는 URL
	 * @param InAccessToken 엑세스 인증 토큰 
	 * @param InContentBody JSON 바디
	 * @param InQueryParams URL 쿼리 패러미터
	 * @param OnGetCustomStruct 결과 콜백
	 * @param bTryTokenRefreshing 토큰 리프레시 시도 여부
	 */
	template<typename StructType>
	void CallApi_Execution(
		const EJWNU_HttpMethod InMethod,
		const EJWNU_ServiceType InServiceType,
		const FString InURL,
		const FString& InAccessToken,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		TFunction<void(const StructType&)> OnGetCustomStruct,
		const bool bTryTokenRefreshing);
	
	/**
	 * 토큰 리프레시 API를 호출하고 성공 시 원래 요청을 재시도하는 함수.
	 * @tparam StructType 원래 요청의 응답 구조체 타입
	 * @param InPendingRequest 재시도할 원래 요청 정보
	 * @param OnGetCustomStruct 최종 결과 콜백
	 */
	template<typename StructType>
	void RefreshTokenAndRetry(
		const FJWNU_PendingApiRequest& InPendingRequest,
		TFunction<void(const StructType&)> OnGetCustomStruct);

	/**
	 * 리프레시 토큰 API URL을 구축하는 함수.
	 * @return 리프레시 토큰 API의 전체 URL
	 */
	FString BuildRefreshTokenURL() const;
};

template <typename StructType>
void UJWNU_GIS_ApiClientService::CallApi(
	const UObject* WorldContextObject, 
	const EJWNU_HttpMethod InMethod,
	const EJWNU_ServiceType InServiceType, 
	const FString InEndpoint, 
	const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams, 
	TFunction<void(const StructType&)> OnGetCustomStruct)
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
			PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("엑세스 토큰 획득 실패!"));
			StructType ErrorResult;
			ErrorResult.Code = TEXT("HOST_NOT_FOUND");
			ErrorResult.Message = TEXT("Failed to get access token from provider");
			OnGetCustomStruct(ErrorResult);
			return;
		}
	}
	
	// API URL 구축
	FString ConstructedURL = ProvidedHost + InEndpoint;
	
	// 토큰 프로바이더에서 엑세스 토큰 획득
	FString ProvidedAccessToken;
	if (const auto TokenProvider = UJWNU_GIS_ApiTokenProvider::Get(WorldContextObject))
	{
		if (TokenProvider->GetAccessToken(InServiceType, ProvidedAccessToken) == false)
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
	
	// 실제 처리
	Self->CallApi_Execution(InMethod, InServiceType, ConstructedURL, ProvidedAccessToken, InContentBody, InQueryParams, OnGetCustomStruct, true);
}

template <typename StructType>
void UJWNU_GIS_ApiClientService::CallApi_Execution(
	const EJWNU_HttpMethod InMethod,
	const EJWNU_ServiceType InServiceType,
	const FString InURL,
	const FString& InAccessToken,
	const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams,
	TFunction<void(const StructType&)> OnGetCustomStruct,
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
		const auto CallbackManage401 = FOnHttpResponseDelegate::CreateWeakLambda(this,
			[this, PendingRequest, OnGetCustomStruct](const FString& ResponseBody)
			{
				// 401 체크를 위해 JSON 파싱
				TSharedPtr<FJsonObject> JsonObject;
				const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);

				if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
				{
					// 커스텀 코드에서 401 관련 코드 확인
					FString CustomCode;
					if (JsonObject->TryGetStringField(TEXT("Code"), CustomCode))
					{
						if (CustomCode == TEXT("UNAUTHORIZED") || CustomCode == TEXT("TOKEN_EXPIRED"))
						{
							PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("401 토큰 만료 감지, 리프레시 시도..."));
							RefreshTokenAndRetry<StructType>(PendingRequest, OnGetCustomStruct);
							return;
						}
					}
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
		UJWNU_GIS_HttpClientHelper::SendReqeust_CustomResponse(GetWorld(), InMethod, InURL, InAccessToken, InContentBody, InQueryParams, CallbackManage401);
	}
	else
	{
		// 401 상태 코드를 처리하지 않는 콜백
		const auto CallbackNoManage401 = FOnHttpResponseDelegate::CreateWeakLambda(this,
			[this, OnGetCustomStruct](const FString& ResponseBody)
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
		UJWNU_GIS_HttpClientHelper::SendReqeust_CustomResponse(GetWorld(), InMethod, InURL, InAccessToken, InContentBody, InQueryParams, CallbackNoManage401);
	}
}

template <typename StructType>
void UJWNU_GIS_ApiClientService::RefreshTokenAndRetry(
	const FJWNU_PendingApiRequest& InPendingRequest,
	TFunction<void(const StructType&)> OnGetCustomStruct)
{
	// 리프레시 토큰 획득
	FString RefreshToken;
	const auto TokenProvider = UJWNU_GIS_ApiTokenProvider::Get(GetWorld());
	if (TokenProvider == nullptr || TokenProvider->GetRefreshToken(InPendingRequest.ServiceType, RefreshToken) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("리프레시 토큰 획득 실패!"));
		StructType ErrorResult;
		ErrorResult.Code = TEXT("REFRESH_TOKEN_NOT_FOUND");
		ErrorResult.Message = TEXT("Failed to get refresh token");
		OnGetCustomStruct(ErrorResult);
		return;
	}

	// 리프레시 콜백
	const auto RefreshCallback = [this, InPendingRequest, OnGetCustomStruct, TokenProvider](const FJWNU_RefreshTokenResponseData& RefreshResult)
	{
		if (RefreshResult.Success && !RefreshResult.AccessToken.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("토큰 리프레시 성공, 원래 요청 재시도..."));

			// 새 토큰 저장
			TokenProvider->UpdateAccessToken(InPendingRequest.ServiceType, RefreshResult.AccessToken);

			// 무한루프 방지를 위해 리프레시 옵션을 해제한 최종 시도
			CallApi_Execution<StructType>(
				InPendingRequest.Method,
				InPendingRequest.ServiceType,
				InPendingRequest.URL,
				RefreshResult.AccessToken,
				InPendingRequest.ContentBody,
				InPendingRequest.QueryParams,
				OnGetCustomStruct,
				false);
		}
		else
		{
			PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("토큰 리프레시 실패: %s"), *RefreshResult.Message);
			StructType ErrorResult;
			ErrorResult.Code = TEXT("TOKEN_REFRESH_FAILED");
			ErrorResult.Message = FString::Printf(TEXT("Token refresh failed: %s"), *RefreshResult.Message);
			OnGetCustomStruct(ErrorResult);
		}
	};
	
	// 리프레시 API 호출 (HandleResponse 템플릿 재사용)
	const FString RefreshURL = BuildRefreshTokenURL();
	const FString RefreshBody = FString::Printf(TEXT("{\"refreshToken\": \"%s\"}"), *RefreshToken);
	CallApi_Execution<FJWNU_RefreshTokenResponseData>(EJWNU_HttpMethod::POST, EJWNU_ServiceType::AuthServer, RefreshURL, {}, RefreshBody, {}, RefreshCallback, false);
}
