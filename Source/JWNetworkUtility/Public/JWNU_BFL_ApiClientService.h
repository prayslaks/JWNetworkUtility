// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNetworkUtilityDelegates.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_BFL_ApiClientService.generated.h"

/**
 * JWNetworkUtility의 각종 기능을 지원하는 블루프린트 함수 라이브러리.
 */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_BFL_ApiClientService : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	/**
	 * [ Blueprint Function Library ] \n Send HTTP Request \n HTTP 리퀘스트를 전송하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InMethod 사용할 HTTP 메서드
	 * @param InURL 호출할 API 엔드포인트  
	 * @param InAuthToken 사용할 인증 엑세스 JWT 토큰 
	 * @param InContentBody 콘텐츠 바디 
	 * @param InQueryParams 쿼리 패러미터
	 * @param InOnHttpResponseBPEvent 리스폰스 바디를 패러미터로 받는 블루프린트 이벤트
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="InQueryParams"))
	static void SendHttpRequest(
		const UObject* WorldContextObject, 
		const EJWNU_HttpMethod InMethod, 
		const FString& InURL, 
		const FString& InAuthToken, 
		const FString& InContentBody, 
		const TMap<FString, FString>& InQueryParams, 
		const FOnHttpResponseBPEvent InOnHttpResponseBPEvent);

	/**
	 * [ Blueprint Function Library ] \n Call API \n API를 호출하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType 사용할 서비스 타입
	 * @param InMethod 사용할 HTTP 메서드
	 * @param InEndpoint API 엔드포인트
	 * @param InContentBody 콘텐츠 바디
	 * @param InQueryParams 쿼리 패러미터
	 * @param InOnHttpResponseBPEvent 리스폰스 바디를 패러미터로 받는 블루프린트 이벤트
	 * @param bRequiresAuth 인증 토큰 요구 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="InQueryParams"))
	static void CallApi(
		const UObject* WorldContextObject,
		const EJWNU_ServiceType InServiceType,
		const EJWNU_HttpMethod InMethod,
		const FString& InEndpoint,
		const FString& InContentBody,
		const TMap<FString, FString>& InQueryParams,
		const FOnHttpResponseBPEvent InOnHttpResponseBPEvent,
		const bool bRequiresAuth);
	
	/**
	 * [ Blueprint Function Library ] \n Load Refresh Token Container from WINDOWS \n 윈도우에 암호화되어 저장된 JWT 인증 리프레시 토큰 컨테이너를 로드하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType Target Service Type \n 특정 서비스 타입
	 * @param OutTokenGetResult Directing Load Result \n 토큰 로드 결과를 나타내는 열거형
	 * @param OutRefreshTokenContainer Loaded Refresh Token Container \n 로드한 리프레시 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenGetResult", BlueprintOutRef="OutRefreshTokenContainer"))
	static void LoadRefreshTokenContainer(
		const UObject* WorldContextObject,
		const EJWNU_ServiceType InServiceType,
		EJWNU_TokenGetResult& OutTokenGetResult,
		FJWNU_RefreshTokenContainer& OutRefreshTokenContainer);

	/**
	 * [ Blueprint Function Library ] \n Save Refresh Token Container to WINDOWS \n JWT 인증 리프레시 토큰 컨테이너를 윈도우에 암호화하여 저장하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenSetResult Directing Save Result \n 토큰 저장 결과를 나타내는 열거형
	 * @param InRefreshTokenContainer Targeting Refresh Token Container \n 저장할 리프레시 토큰 컨테이너
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenSetResult"))
	static void SaveRefreshTokenContainer(
		const UObject* WorldContextObject,
		const EJWNU_ServiceType InServiceType,
		EJWNU_TokenSetResult& OutTokenSetResult,
		const FJWNU_RefreshTokenContainer& InRefreshTokenContainer);
	
	/**
	 * [ Blueprint Function Library ] \n Get Access Token Container \n JWT 인증 엑세스 토큰 컨테이너를 획득하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenGetResult Directing Get Result \n 획득 결과를 나타내는 열거형
	 * @param OutAccessTokenContainer Extracted Access Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 엑세스 토큰 컨테이너
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenGetResult", BlueprintOutRef="OutAccessTokenContainer"))
	static void GetAccessTokenContainer(
		const UObject* WorldContextObject, 
		const EJWNU_ServiceType InServiceType, 
		EJWNU_TokenGetResult& OutTokenGetResult, 
		FJWNU_AccessTokenContainer& OutAccessTokenContainer);

	/**
	 * [ Blueprint Function Library ] \n Set Access Token Container \n JWT 인증 엑세스 토큰 컨테이너를 설정하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenSetResult 설정 결과를 나타내는 열거형
	 * @param InAccessTokenContainer New Access Token Container \n 새로운 JWT 인증 토큰 컨테이너
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenSetResult"))
	static void SetAccessTokenContainer(
		const UObject* WorldContextObject,
		const EJWNU_ServiceType InServiceType,
		EJWNU_TokenSetResult& OutTokenSetResult,
		const FJWNU_AccessTokenContainer& InAccessTokenContainer);

	/**
	 * [ Blueprint Function Library ] \n Get Host \n 목표 호스트 주소를 획득하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutHost Host Address \n 호스트 주소
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static bool GetHost(
		const UObject* WorldContextObject, 
		const EJWNU_ServiceType InServiceType, 
		FString& OutHost);

	/**
	 * [ Blueprint Function Library ] \n Get User Id \n 현재 로그인된 사용자 ID를 획득하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @return UserId 문자열
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static FString GetUserId(const UObject* WorldContextObject);

	/**
	 * [ Blueprint Function Library ] \n Set User Id \n 사용자 ID를 설정하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InUserId 설정할 UserId
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static void SetUserId(const UObject* WorldContextObject, const FString& InUserId);

	/**
	 * [ Blueprint Function Library ] \n Clear Session \n 특정 서비스 타입의 인증 정보를 정리하는 함수. (AccessToken 초기화, UserId 유지)
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType 정리할 서비스 타입
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static void ClearSession(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType);

	/**
	 * [ Blueprint Function Library ] \n Convert JSON Object String to Unreal Struct \n 입력한 JSON 바디 문자열을 와일드카드로 지정한 언리얼 구조체로 파싱을 시도하는 노드 정의.
	 * @param JsonString 파싱하길 원하는 JSON 바디 문자열
	 * @param OutConvertResult 파싱 결과를 나타내는 열거형
	 * @param OutStruct 파싱하길 원하는 언리얼 구조체 와일드 카드
	 * @return 파싱 성공 여부
	 */
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

	/**
	 * [ Blueprint Function Library ] \n Convert Unreal Struct to JSON Object String \n 와일드카드로 지정한 언리얼 구조체를 JSON 문자열로 변환을 시도하는 노드 정의.
	 * @param InStruct 변환하길 원하는 언리얼 구조체 와일드 카드
	 * @param OutConvertResult 변환 결과를 나타내는 열거형
	 * @param OutJsonString 변환된 JSON 문자열
	 * @return 변환 성공 여부
	 */
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
	
	UFUNCTION(BlueprintCallable, Category = "JSON|Format")
	static FString FormatJsonWithTemplate(const FString Template, FString JsonData);
};
