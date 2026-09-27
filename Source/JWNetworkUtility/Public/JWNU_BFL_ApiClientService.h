// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNetworkUtilityDelegates.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Engine/Engine.h"
#include "JWNU_BFL_ApiClientService.generated.h"

class UJWNU_ApiRequest;
class UJWNU_HttpRequest;

/** JWNetworkUtility의 각종 기능을 지원하는 블루프린트 함수 라이브러리. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_BFL_ApiClientService : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/** 콜백을 먼저 연결하고 전체 URL로 HTTP를 즉시 전송하는 함수. 반환 요청은 선택적 취소·조회용이다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|HTTP", meta=(WorldContext="WorldContextObject", DisplayName="Send HTTP Request", AutoCreateRefTerm="InQueryParams,InOnHttpRequestJobRetry"))
	static UJWNU_HttpRequest* SendHttpRequest(const UObject* WorldContextObject, EJWNU_HttpMethod InMethod,
		const FString& InURL, UPARAM(DisplayName="API Key") const FString& ApiKey, const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams, const FOnHttpResponseBPEvent& InOnHttpResponse,
		const FOnHttpRequestJobRetryBPEvent& InOnHttpRequestJobRetry);

	/** 콜백을 먼저 연결하고 API를 즉시 호출하는 함수. 반환 요청은 선택적 취소·조회용이며 Start를 다시 호출하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|API", meta=(WorldContext="WorldContextObject", DisplayName="Call API", AutoCreateRefTerm="InQueryParams,InOnHttpRequestJobRetry"))
	static UJWNU_ApiRequest* CallApi(const UObject* WorldContextObject, EJWNU_ServiceType InServiceType,
		EJWNU_HttpMethod InMethod, const FString& InEndpoint, const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams, const FOnHttpResponseBPEvent& InOnHttpResponse,
		const FOnHttpRequestJobRetryBPEvent& InOnHttpRequestJobRetry, bool bRequiresAuth = true);

	/** 서비스별로 암호화 저장된 리프레시 토큰을 복호화해 읽는 함수. 로드 결과를 BP 실행 핀으로 전달한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenGetResult", BlueprintOutRef="OutRefreshTokenContainer"))
	static void LoadRefreshTokenContainer(
		const UObject* WorldContextObject,
		const EJWNU_ServiceType InServiceType,
		EJWNU_TokenGetResult& OutTokenGetResult,
		FJWNU_RefreshTokenContainer& OutRefreshTokenContainer);

	/** 리프레시 토큰 컨테이너를 Windows DPAPI로 암호화 저장하고 저장 결과를 BP 실행 핀으로 전달하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenSetResult"))
	static void SaveRefreshTokenContainer(
		const UObject* WorldContextObject,
		const EJWNU_ServiceType InServiceType,
		EJWNU_TokenSetResult& OutTokenSetResult,
		const FJWNU_RefreshTokenContainer& InRefreshTokenContainer);
	
	/** 서비스별 메모리 액세스 토큰과 조회 결과를 반환하는 함수. 조회 성공이 토큰 유효성을 보장하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenGetResult", BlueprintOutRef="OutAccessTokenContainer"))
	static void GetAccessTokenContainer(
		const UObject* WorldContextObject, 
		const EJWNU_ServiceType InServiceType, 
		EJWNU_TokenGetResult& OutTokenGetResult, 
		FJWNU_AccessTokenContainer& OutAccessTokenContainer);

	/** 서비스별 액세스 토큰과 만료 시각을 메모리에 교체하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenSetResult"))
	static void SetAccessTokenContainer(
		const UObject* WorldContextObject,
		const EJWNU_ServiceType InServiceType,
		EJWNU_TokenSetResult& OutTokenSetResult,
		const FJWNU_AccessTokenContainer& InAccessTokenContainer);

	/** 서비스별 호스트 조회 성공 여부와 주소를 반환하는 함수. 등록된 주소가 비어 있어도 true일 수 있다. */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static bool GetHost(
		const UObject* WorldContextObject, 
		const EJWNU_ServiceType InServiceType, 
		FString& OutHost);

	/** 현재 GameInstance 메모리에 보관된 로그인 사용자 식별자를 반환하는 함수. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static FString GetUserId(const UObject* WorldContextObject);

	/** 현재 GameInstance 메모리의 로그인 사용자 식별자를 설정하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static void SetUserId(const UObject* WorldContextObject, const FString& InUserId);

	/** 지정 서비스의 액세스 토큰을 비우는 함수. 사용자 식별자와 저장된 리프레시 토큰은 유지한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static void ClearSession(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType);

	/** JSON 객체를 연결된 BP 구조체로 변환하고 성공 여부와 상세 변환 결과를 반환하는 함수. */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="JWNU Blueprint Function Library", meta=(CustomStructureParam="OutStruct"))
	static bool ConvertJsonStringToStruct(const FString& JsonString, EJWNU_ConvertJsonToStructResult& OutConvertResult, int32& OutStruct);
	static bool Generic_ConvertJsonStringToStruct(const FString& JsonString, EJWNU_ConvertJsonToStructResult& OutConvertResult, const FProperty* StructProperty, void* StructPtr);
	DECLARE_FUNCTION(execConvertJsonStringToStruct)
	{
		// JSON 객체 문자열
		P_GET_PROPERTY(FStrProperty, JsonString);

		// 파싱 결과를 받을 참조 패러미터
		Stack.StepCompiledIn<FProperty>(nullptr);
		EJWNU_ConvertJsonToStructResult* OutConvertResultPtr = reinterpret_cast<EJWNU_ConvertJsonToStructResult*>(Stack.MostRecentPropertyAddress);

		// 파싱된 구조체를 받을 참조 패러미터
		Stack.StepCompiledIn<FProperty>(nullptr);
		void* StructPtr = Stack.MostRecentPropertyAddress;
		const FProperty* StructProperty = Stack.MostRecentProperty;

		P_FINISH;

		// 변환 결과(Success/Fail)를 블루프린트 리턴값으로 전달
		*static_cast<bool*>(RESULT_PARAM) = Generic_ConvertJsonStringToStruct(JsonString, *OutConvertResultPtr, StructProperty, StructPtr);
	}

	/** 연결된 BP 구조체를 JSON 문자열로 변환하고 성공 여부와 상세 변환 결과를 반환하는 함수. */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="JWNU Blueprint Function Library", meta=(CustomStructureParam="InStruct"))
	static bool ConvertStructToJsonString(const int32& InStruct, EJWNU_ConvertStructToJsonResult& OutConvertResult, FString& OutJsonString);
	static bool Generic_ConvertStructToJsonString(const FProperty* StructProperty, const void* StructPtr, EJWNU_ConvertStructToJsonResult& OutConvertResult, FString& OutJsonString);
	DECLARE_FUNCTION(execConvertStructToJsonString)
	{
		// 변환할 구조체를 받을 패러미터
		Stack.StepCompiledIn<FProperty>(nullptr);
		const void* StructPtr = Stack.MostRecentPropertyAddress;
		const FProperty* StructProperty = Stack.MostRecentProperty;

		// 변환 결과를 받을 참조 패러미터
		Stack.StepCompiledIn<FProperty>(nullptr);
		EJWNU_ConvertStructToJsonResult* OutConvertResultPtr = reinterpret_cast<EJWNU_ConvertStructToJsonResult*>(Stack.MostRecentPropertyAddress);

		// 변환된 JSON 문자열을 받을 참조 패러미터
		P_GET_PROPERTY_REF(FStrProperty, OutJsonString);

		P_FINISH;

		// 변환 결과(Success/Fail)를 블루프린트 리턴값으로 전달
		*static_cast<bool*>(RESULT_PARAM) = Generic_ConvertStructToJsonString(StructProperty, StructPtr, *OutConvertResultPtr, OutJsonString);
	}
};
