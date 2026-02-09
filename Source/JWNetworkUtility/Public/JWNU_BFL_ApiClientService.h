// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNU_GIS_ApiTokenProvider.h"
#include "JWNU_GIS_HttpClientHelper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_BFL_ApiClientService.generated.h"

/**
 * HTTP 리스폰스 바디를 받을 수 있도록 포장하는 다이나믹 델리게이트
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnHttpResponseBP, const FString&, HttpResponseBody);

/**
 * JWNetworkUtility의 각종 기능을 지원하는 블루프린트 함수 라이브러리.
 */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_BFL_ApiClientService : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	/**
	 * Send Http Request \n HTTP 리퀘스트를 전송하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InMethod Wanted HTTP Method : 사용할 HTTP 메서드
	 * @param InURL Api Endpoint : 호출할 API 엔드포인트  
	 * @param InAuthToken Bearer Token : 사용할 인증 토큰 
	 * @param InContentBody Content Body : 콘텐츠 바디 
	 * @param InQueryParams Query Parameters : 쿼리 패러미터
	 * @param InOnHttpResponseBP Blueprint Custom Event : 블루프린트 이벤트 
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="InQueryParams"))
	static void SendHttpRequest(
		const UObject* WorldContextObject, 
		const EJWNU_HttpMethod InMethod, 
		const FString& InURL, 
		const FString& InAuthToken, 
		const FString& InContentBody, 
		const TMap<FString, FString>& InQueryParams, 
		const FOnHttpResponseBP InOnHttpResponseBP);
	
	/**
	 * Get Access Token from Container \n 컨테이너로부터 JWT 인증 엑세스 토큰을 획득하는 함수.
	 * @param WorldContextObject
	 * @param InServiceType Target Service Type \n 특정 서비스 타입
	 * @param OutTokenExtractionResult Directing Extraction Result \n 토큰 추출 결과를 나타내는 열거형
	 * @param OutAccessToken Extracted Access Token \n 특정 서비스 타입과 매핑되는 JWT 인증 엑세스 토큰
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenExtractionResult", BlueprintOutRef="OutAccessToken"))
	static void GetAccessToken(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, EJWNU_TokenExtractionResult& OutTokenExtractionResult, FString& OutAccessToken);
	
	/**
	 * Get Refresh Token from Container \n 컨테이너로부터 JWT 인증 엑세스 토큰을 획득하는 함수.
	 * @param WorldContextObject
	 * @param InServiceType Target Service Type \n 특정 서비스 타입
	 * @param OutTokenExtractionResult Directing Extraction Result \n 토큰 추출 결과를 나타내는 열거형
	 * @param OutRefreshToken Extracted Refresh Token \n 특정 서비스 타입과 매핑되는 JWT 인증 엑세스 토큰
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", ExpandEnumAsExecs="OutTokenExtractionResult", BlueprintOutRef="OutRefreshToken"))
	static void GetRefreshToken(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, EJWNU_TokenExtractionResult& OutTokenExtractionResult, FString& OutRefreshToken);
	
	/**
	 * Get Token Container that has Info \n JWT 인증 토큰 컨테이너를 획득하는 함수.
	 * @param WorldContextObject
	 * @param InServiceType Target Service Type \n 특정 서비스 타입
	 * @param OutTokenContainer Extracted Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", BlueprintOutRef="OutTokenContainer"))
	static bool GetTokenContainer(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, FJWNU_AccessTokenContainer& OutTokenContainer);
	
	/**
	 * 입력한 JSON 바디 문자열을 와일드카드로 지정한 언리얼 구조체로 파싱을 시도하는 노드 정의.
	 * @param JsonString 파싱하길 원하는 JSON 바디 문자열
	 * @param OutStruct 파싱하길 원하는 언리얼 구조체 와일드 카드
	 * @return 파싱 성공 여부
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="JWNU Blueprint Function Library", meta=(CustomStructureParam="OutStruct"))
	static bool ConvertJsonStringToStruct(const FString& JsonString, int32& OutStruct);
	static bool Generic_ConvertJsonStringToStruct(const FString& JsonString, const FProperty* StructProperty, void* StructPtr);
	DECLARE_FUNCTION(execConvertJsonStringToStruct)
	{
		P_GET_PROPERTY(FStrProperty, JsonString);
		Stack.StepCompiledIn<FProperty>(nullptr);
		void* StructPtr = Stack.MostRecentPropertyAddress;
		const FProperty* StructProperty = Stack.MostRecentProperty;
		P_FINISH;

		// 변환 결과(Success/Fail)를 블루프린트 리턴값으로 전달
		*static_cast<bool*>(RESULT_PARAM) = Generic_ConvertJsonStringToStruct(JsonString, StructProperty, StructPtr);
	}

	/**
	 * 입력받은 토큰을 암호화하여 저장하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType 
	 * @param InToken 암호화할 리프레시 토큰
	 * @return 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject"))
	static bool SaveTokenWithEncryption(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, const FString& InToken);

	/**
	 * 암호화된 토큰을 복호화하여 반환하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @param InServiceType 
	 * @param OutToken 복호화한 토큰
	 * @return 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU Blueprint Function Library", meta=(WorldContext="WorldContextObject", BlueprintOutRef="OutToken"))
	static bool LoadTokenWithDecryption(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, FString& OutToken);

private:
	/**
	 * 블루프린트 커스텀 이벤트를 네이티브 콜백 델리게이트로 감싸서 반환하는 함수.
	 * @param OnHttpResponseBP 감싸고 싶은 블루프린트 커스텀 이벤트 델리게이트
	 * @return 블루프린트 커스텀 이벤트를 감싸고 있는 네이티브 콜백 델리게이트
	 */
	static FOnHttpResponseDelegate WrapOnHttpResponseBP(FOnHttpResponseBP OnHttpResponseBP);
};
