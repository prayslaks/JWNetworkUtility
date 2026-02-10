// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWNU_GIS_ApiIdentityProvider.generated.h"

/**
 * 클래스 전용의 로그 카테고리 선언
 */
JWNETWORKUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_ApiIdentityProvider, Log, All);

/**
 * 인증 JWT 엑세스 토큰, 리프레시 토큰, UserId를 관리하는 게임인스턴스 서브시스템.
 * 서비스 타입별로 다른 엑세스 토큰을 TMap으로 관리하며, 리프레시 토큰을 암호화-복호화한다.
 * UserId는 단일 FString(메모리 전용, 로그인/리프레시 응답에서 수신)으로 관리한다.
 */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_ApiIdentityProvider : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// 프로그래밍 팁 : meta=(BlueprintOutRef="OutRef1, OutRef2, ...")를 사용하면 블루프린트 활용도를 높일 수 있다

	/**
	 * Overriding for initializing token container map \n 토큰 컨테이너 맵을 초기화하는 로직 오버라이드.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * 외부에서 HTTP 클라이언트 헬퍼 서브시스템을 획득하기 위해 호출하는 함수.
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @return UJWNU_GIS_ApiIdentityProvider 게임인스턴스 서브시스템
	 */
	static UJWNU_GIS_ApiIdentityProvider* Get(const UObject* WorldContextObject);

	/**
	 * Get Access Token Container \n JWT 인증 엑세스 토큰 컨테이너를 획득하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenGetResult Directing Get Result \n 획득 결과를 나타내는 열거형
	 * @param OutAccessTokenContainer Extracted Access Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 엑세스 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization", meta=(BlueprintOutRef="OutTokenGetResult, OutAccessTokenContainer"))
	bool GetAccessTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenGetResult& OutTokenGetResult, FJWNU_AccessTokenContainer& OutAccessTokenContainer) const;

	/**
	 * Get Access Token Container \n JWT 인증 엑세스 토큰 컨테이너를 획득하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutAccessTokenContainer Extracted Access Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 엑세스 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	bool GetAccessTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_AccessTokenContainer& OutAccessTokenContainer) const;

	/**
	 * Set Access Token Container \n JWT 인증 엑세스 토큰 컨테이너를 설정하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenSetResult 설정 결과를 나타내는 열거형
	 * @param InAccessTokenContainer New Access Token Container \n 새로운 JWT 인증 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization", meta=(BlueprintOutRef="OutTokenSetResult"))
	bool SetAccessTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenSetResult& OutTokenSetResult, const FJWNU_AccessTokenContainer& InAccessTokenContainer);

	/**
	 * Set Access Token Container \n JWT 인증 엑세스 토큰 컨테이너를 설정하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param InTokenContainer New Access Token Container \n 새로운 JWT 인증 토큰 컨테이너
	 * @return
	 */
	bool SetAccessTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_AccessTokenContainer& InTokenContainer);

	/**
	 * Load Refresh Token Container from WINDOWS \n 윈도우를 통해 JWT 인증 리프레시 토큰 컨테이너를 로드하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenGetResult Directing Load Result \n 토큰 추출 결과를 나타내는 열거형
	 * @param OutRefreshTokenContainer Loaded Refresh Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 리프레시 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization", meta=(BlueprintOutRef="OutTokenGetResult, OutRefreshTokenContainer"))
	bool GetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenGetResult& OutTokenGetResult, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer) const;

	/**
	 * [ For CPP ] \n Load Refresh Token Container from WINDOWS \n 윈도우를 통해 JWT 인증 리프레시 토큰 컨테이너를 로드하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutRefreshTokenContainer Loaded Refresh Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 리프레시 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	static bool GetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer);

	/**
	 * Save Refresh Token Container to WINDOWS \n 윈도우를 통해 JWT 인증 리프레시 토큰 컨테이너를 저장하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutTokenSetResult Directing Save Result \n 토큰 추출 결과를 나타내는 열거형
	 * @param InRefreshTokenContainer Targeting Refresh Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 리프레시 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Authorization", meta=(BlueprintOutRef="OutTokenSetResult"))
	bool SetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenSetResult& OutTokenSetResult, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer);

	/**
	 * [ For CPP ] \n Save Refresh Token Container to WINDOWS \n 윈도우를 통해 JWT 인증 리프레시 토큰 컨테이너를 저장하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param InRefreshTokenContainer Targeting Refresh Token Container \n 특정 서비스 타입과 매핑되는 JWT 인증 리프레시 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	static bool SetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer);

	// ──────── UserId ────────

	/**
	 * 현재 로그인된 사용자 ID를 반환한다. (메모리 전용, 로그인/리프레시 응답에서 수신)
	 * @return UserId 문자열
	 */
	FString GetUserId() const;

	/**
	 * 사용자 ID를 설정한다. (메모리 전용)
	 * @param InUserId 설정할 UserId
	 */
	void SetUserId(const FString& InUserId);

	// ──────── 세션 정리 ────────

	/**
	 * 특정 서비스 타입의 인증 정보를 정리한다. (AccessToken 초기화, UserId 유지)
	 * @param InServiceType 정리할 서비스 타입
	 */
	void ClearSession(EJWNU_ServiceType InServiceType);

private:

	/**
	 * 특정 서비스 타입의 리프레시 토큰 컨테이너를 암호화하여 저장하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param InRefreshTokenContainer Targeting Refresh Token Container \n 저장할 리프레시 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	static bool SaveRefreshTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer);

	/**
	 * 특정 서비스 타입의 리프레시 토큰 컨테이너를 복호화하여 반환하는 함수.
	 * @param InServiceType Targeting Service Type \n 특정 서비스 타입
	 * @param OutRefreshTokenContainer Targeting Refresh Token Container \n 로드할 리프레시 토큰 컨테이너
	 * @return Success or Fail \n 성공 여부
	 */
	static bool LoadRefreshTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer);

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

	/**
	 * 현재 로그인된 사용자 ID. (메모리 전용)
	 */
	FString UserId;

};
