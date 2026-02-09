// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWNU_GIS_ApiTokenProvider.generated.h"

/**
 * 클래스 전용의 로그 카테고리 선언
 */
DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_ApiTokenProvider, Log, All);

/**
 * 특정 서비스 타입을 지정하는 열거형. JWNetworkUtility에서 API 호출 편의성을 극대화하기 위한 논리적인 방책이다. 서비스 타입에 따라서 사용하는 호스트 URL과 토큰이 달라진다.
 */
UENUM(BlueprintType)
enum class EJWNU_ServiceType : uint8
{
	GameServer,
	AuthServer,
	Platform,
	External,
};

/**
 * JWNU_GIS_ApiTokenProvider에서 토큰 획득 시도의 결과를 지정하는 열거형. 블루프린트 지원에 활용된다.
 */
UENUM(BlueprintType)
enum class EJWNU_TokenExtractionResult : uint8
{
	Fail = 0,
	Success = 1,
	Empty = 2,	
};

/**
 * 엑세스 토큰 값과 해당 토큰의 만료 시간을 저장하는 언리얼 구조체.
 */
USTRUCT(BlueprintType)
struct FJWNU_AccessTokenContainer
{
	GENERATED_BODY()

	/**
	 * 인증 JWT 엑세스 토큰 값. 거의 대부분의 API에 Authorization으로 활용하며 서버에 의해 만료 시간이 관리된다.
	 */
	UPROPERTY()
	FString AccessToken = TEXT("");
	
	/**
	 * 엑세스 토큰의 만료 예정 시간을 유닉스 타임스탬프를 활용해, 클라이언트 단에서 판단할 수 있도록 한다.
	 * 단, 30초 정도의 버퍼 타임을 통해서 엑세스 토큰이 왠만하면 살아남을 수 있도록 관리한다.
	 */
	UPROPERTY()
	int64 ExpiresAt = 0;
};

/**
 * 
 */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_ApiTokenProvider : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	// 프로그래밍 팁 : meta=(BlueprintOutRef="OutRef1, OutRef2, ...")를 사용하면 블루프린트 활용도를 높일 수 있다
	
	/**
	 * 외부에서 HTTP 클라이언트 헬퍼 서브시스템을 획득하기 위해 호출하는 함수. (For CPP)
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @return UJW_GIS_HttpClientHelper HTTP 클라이언트 헬퍼 서브시스템
	 */
	static UJWNU_GIS_ApiTokenProvider* Get(const UObject* WorldContextObject);

	/**
	 * Get Access Token from Container \n 컨테이너로부터 JWT 인증 엑세스 토큰을 획득하는 함수. (For CPP)
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutAccessToken Extracted Access Token \n 특정 서비스 타입과 매핑되는 JWT 인증 엑세스 토큰
	 * @return Success or Fail \n 성공 여부
	 */
	bool GetAccessToken(const EJWNU_ServiceType InServiceType, FString& OutAccessToken) const;
	
	/**
	 * Get Access Token from Container \n 컨테이너로부터 JWT 인증 엑세스 토큰을 획득하는 함수. (For Blueprint)
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenExtractionResult Directing Extraction Result \n 토큰 추출 결과를 나타내는 열거형
	 * @param OutAccessToken Extracted Access Token \n 특정 서비스 타입과 매핑되는 JWT 인증 엑세스 토큰
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization", meta=(BlueprintOutRef="OutTokenExtractionResult, OutAccessToken"))
	bool GetAccessToken(const EJWNU_ServiceType InServiceType, EJWNU_TokenExtractionResult& OutTokenExtractionResult, FString& OutAccessToken) const;

	/**
	 * Get Refresh Token from Container \n 컨테이너로부터 JWT 인증 리프레시 토큰을 획득하는 함수. (For Blueprint)
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenExtractionResult Directing Extraction Result \n 토큰 추출 결과를 나타내는 열거형
	 * @param OutRefreshToken Extracted Refresh Token \n 특정 서비스 타입과 매핑되는 JWT 인증 리프레시 토큰
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization", meta=(BlueprintOutRef="OutTokenExtractionResult, OutRefreshToken"))
	bool GetRefreshToken(const EJWNU_ServiceType InServiceType, EJWNU_TokenExtractionResult& OutTokenExtractionResult, FString& OutRefreshToken) const;
	
	/**
	 * Get Refresh Token from Container \n 컨테이너로부터 JWT 인증 리프레시 토큰을 획득하는 함수. (For CPP)
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutRefreshToken Extracted Refresh Token \n 특정 서비스 타입과 매핑되는 JWT 인증 리프레시 토큰
	 * @return Success or Fail \n 성공 여부
	 */
	bool GetRefreshToken(const EJWNU_ServiceType InServiceType, FString& OutRefreshToken) const;
	
	/**
	 * Get Token Container that has Info \n JWT 인증 토큰 컨테이너를 획득하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenContainer Extracted Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization", meta=(BlueprintOutRef="OutTokenContainer"))
	bool GetTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_AccessTokenContainer& OutTokenContainer) const;

	/**
	 * Update Token Container \n JWT 인증 토큰 컨테이너를 갱신하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param InTokenContainer New Token Container \n 새로운 JWT 인증 토큰 컨테이너
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization")
	void UpdateTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_AccessTokenContainer& InTokenContainer);

	/**
	 * Update Access Token Only \n 엑세스 토큰만 갱신하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param InAccessToken New Access Token \n 새로운 엑세스 토큰
	 * @return Success or Fail \n 성공 여부 (해당 서비스 타입의 컨테이너가 존재해야 함)
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization")
	bool UpdateAccessToken(const EJWNU_ServiceType InServiceType, const FString& InAccessToken);

	/**
	 * 특정 서비스 타입의 리프레시 토큰을 암호화하여 저장하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param InRefreshToken Targeting Refresh Token \n 저장할 리프레시 토큰
	 * @return Success or Fail \n 성공 여부
	 */
	static bool SaveRefreshTokenWithEncryption(const EJWNU_ServiceType InServiceType, const FString& InRefreshToken);

	/**
	 * 특정 서비스 타입의 리프레시 토큰을 복호화하여 반환하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutRefreshToken Targeting Refresh Token \n 저장할 리프레시 토큰
	 * @return Success or Fail \n 성공 여부
	 */
	static bool LoadRefreshTokenWithDecryption(const EJWNU_ServiceType InServiceType, FString& OutRefreshToken);

private:
	
	/**
	 * 입력 토큰을 암호화하여 반환하는 함수.
	 * @param InToken Targeting Token \n 암호화할 토큰
	 * @param OutEncryptedData Data After Encryption \n 암호화된 토큰 데이터
	 * @return Success or Fail \n 성공 여부
	 */
	static bool EncryptToken(const FString& InToken, TArray<uint8>& OutEncryptedData);

	/**
	 * 암호화된 토큰 데이터를 복호화하여 반환하는 함수.
	 * @param InEncryptedData Targeting Encrypted Token Data \n 암호화되어 있는 토큰 데이터
	 * @param OutToken Token After Decryption \n 복호화된 토큰 데이터
	 * @return Success or Fail \n 성공 여부
	 */
	static bool DecryptToken(const TArray<uint8>& InEncryptedData, FString& OutToken);
	
	/**
	 * 특정 서비스 타입과 JWT 인증 토큰 컨테이너를 매핑하는 맵.
	 */
	UPROPERTY()
	TMap<EJWNU_ServiceType, FJWNU_AccessTokenContainer> ServiceTypeToTokenContainerMap;
	
};
