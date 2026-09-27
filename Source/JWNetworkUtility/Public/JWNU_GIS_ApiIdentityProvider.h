// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Engine.h"
#include "JWNU_GIS_ApiIdentityProvider.generated.h"

/** 클래스 전용의 로그 카테고리 선언 */
JWNETWORKUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_ApiIdentityProvider, Log, All);

/** 서비스별 액세스 토큰과 로그인 사용자 ID를 메모리에 보관하고 Windows DPAPI 기반 리프레시 토큰 저장·복원을 제공한다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_ApiIdentityProvider : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	// 프로그래밍 팁 : meta=(BlueprintOutRef="OutRef1, OutRef2, ...")를 사용하면 블루프린트 활용도를 높일 수 있다

	/** 서비스별 기본 액세스 토큰 컨테이너를 초기화하는 함수. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 월드의 GameInstance에서 인증 정보 서브시스템을 반환하는 함수. 유효한 월드가 없으면 null이다. */
	static UJWNU_GIS_ApiIdentityProvider* Get(const UObject* WorldContextObject);

	/** 서비스별 액세스 토큰 컨테이너를 조회하는 함수. true는 항목 존재 여부이며 토큰 내용·만료 유효성을 보장하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtility|Authorization", meta=(BlueprintOutRef="OutTokenGetResult, OutAccessTokenContainer"))
	bool GetAccessTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenGetResult& OutTokenGetResult, FJWNU_AccessTokenContainer& OutAccessTokenContainer) const;

	/** 서비스별 액세스 토큰 컨테이너를 조회하는 함수. true는 항목 존재 여부이며 토큰 내용·만료 유효성을 보장하지 않는다. */
	bool GetAccessTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_AccessTokenContainer& OutAccessTokenContainer) const;

	/** 서비스별 액세스 토큰 컨테이너를 메모리에 추가하거나 교체하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtility|Authorization", meta=(BlueprintOutRef="OutTokenSetResult"))
	bool SetAccessTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenSetResult& OutTokenSetResult, const FJWNU_AccessTokenContainer& InAccessTokenContainer);

	/** 서비스별 액세스 토큰 컨테이너를 메모리에 추가하거나 교체하는 함수. */
	bool SetAccessTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_AccessTokenContainer& InTokenContainer);

	/** 서비스별 저장 파일을 읽고 복호화해 리프레시 토큰 컨테이너를 반환하는 함수. 서버 인증 유효성은 별도로 확인한다. */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtility|Authorization", meta=(BlueprintOutRef="OutTokenGetResult, OutRefreshTokenContainer"))
	bool GetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenGetResult& OutTokenGetResult, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer) const;

	/** 서비스별 저장 파일을 읽고 복호화해 리프레시 토큰 컨테이너를 반환하는 함수. 서버 인증 유효성은 별도로 확인한다. */
	static bool GetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer);

	/** 서비스별 리프레시 토큰을 Windows DPAPI로 암호화 저장하고 성공 여부를 반환하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtility|Authorization", meta=(BlueprintOutRef="OutTokenSetResult"))
	bool SetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenSetResult& OutTokenSetResult, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer);

	/** 서비스별 리프레시 토큰을 Windows DPAPI로 암호화 저장하고 성공 여부를 반환하는 함수. */
	static bool SetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer);

	// ──────── UserId ────────

	/** 메모리에만 보관한 현재 로그인 사용자 식별자를 반환하는 함수. */
	FString GetUserId() const;

	/** 로그인·토큰 갱신 응답에서 받은 사용자 식별자를 메모리에 설정하는 함수. */
	void SetUserId(const FString& InUserId);

	// ──────── 세션 정리 ────────

	/** 지정 서비스의 액세스 토큰을 비우는 함수. 사용자 식별자와 저장된 리프레시 토큰은 유지한다. */
	void ClearSession(EJWNU_ServiceType InServiceType);

private:

	/** 리프레시 토큰 컨테이너를 JSON으로 직렬화하고 암호화해 서비스별 파일에 저장하는 함수. */
	static bool SaveRefreshTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer);

	/** 서비스별 저장 파일을 복호화하고 JSON을 리프레시 토큰 컨테이너로 복원하는 함수. */
	static bool LoadRefreshTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer);

	/** Windows DPAPI와 장치 기반 추가 엔트로피로 문자열을 암호화하는 함수. */
	static bool EncryptToken(const FString& InToken, TArray<uint8>& OutEncryptedData);

	/** Windows DPAPI와 저장 시 사용한 장치 기반 추가 엔트로피로 문자열을 복호화하는 함수. */
	static bool DecryptToken(const TArray<uint8>& InEncryptedData, FString& OutToken);

	/** 특정 서비스 타입과 JWT 인증 토큰 컨테이너를 매핑하는 맵. */
	UPROPERTY()
	TMap<EJWNU_ServiceType, FJWNU_AccessTokenContainer> ServiceTypeToTokenContainerMap;

	/** 현재 로그인된 사용자 ID. (메모리 전용) */
	FString UserId;

};
