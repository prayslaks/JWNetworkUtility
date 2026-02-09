// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWNU_GIS_ApiTokenProvider.h"
#include "JWNU_GIS_ApiHostProvider.generated.h"

/**
 * 클래스 전용의 로그 카테고리 선언
 */
DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_ApiHostProvider, Log, All);

/**
 * JWNU_GIS_ApiHostProvider에서 호스트 획득 시도의 결과를 지정하는 열거형. 블루프린트 지원에 활용된다.
 */
UENUM(BlueprintType)
enum class EJWNU_HostExtractionResult : uint8
{
	Fail = 0,
	Success = 1,
	Empty = 2,	
};

/**
 * DefaultJWNetworkUtility.ini 설정값을 CPP와 블루프린트로 읽을 수 있도록 돕는 게임인스턴스 서브시스템.
 * 서비스 타입별로 다른 호스트 URL을 TMap으로 관리한다.
 */
UCLASS(Config=JWNetworkUtility)
class JWNETWORKUTILITY_API UJWNU_GIS_ApiHostProvider : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * 외부에서 게임 인스턴스 API 서브시스템을 반환하는 함수. (For CPP)
	 * @param WorldContextObject 월드 컨텍스트 오브젝트
	 * @return
	 */
	static UJWNU_GIS_ApiHostProvider* Get(const UObject* WorldContextObject);
	
	/**
	 * 서비스 타입에 매핑된 호스트를 반환하는 함수. (For CPP)
	 * @param InServiceType 조회할 서비스 타입
	 * @param OutHost API 서버 호스트 주소
	 * @return 호스트 URL (매핑이 없으면 빈 문자열)
	 */
	bool GetHost(const EJWNU_ServiceType InServiceType, FString& OutHost) const;
	
	/**
	 * 서비스 타입에 매핑된 호스트를 반환하는 함수. (For Blueprint)
	 * @param InServiceType 조회할 서비스 타입
	 * @param OutHostExtractionResult
	 * @param OutHost API 서버 호스트 주소
	 * @return 호스트 URL (매핑이 없으면 빈 문자열)
	 */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtiltiy|Configuration")
	bool GetHost(const EJWNU_ServiceType InServiceType, EJWNU_HostExtractionResult& OutHostExtractionResult, FString& OutHost) const;

protected:

	/**
	 * 서비스 타입별 호스트 URL 매핑.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "JWNetworkUtiltiy|Configuration")
	TMap<EJWNU_ServiceType, FString> ServiceTypeToHostMap;
};
