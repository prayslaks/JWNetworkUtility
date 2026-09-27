// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Engine.h"
#include "JWNU_GIS_ApiHostProvider.generated.h"

/** 클래스 전용의 로그 카테고리 선언 */
JWNETWORKUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_GIS_ApiHostProvider, Log, All);

/** DefaultJWNetworkUtility.ini 설정값을 CPP와 블루프린트로 읽을 수 있도록 돕는 게임인스턴스 서브시스템. 서비스 타입별로 다른 호스트 URL을 TMap으로 관리한다. */
UCLASS(Config=JWNetworkUtility)
class JWNETWORKUTILITY_API UJWNU_GIS_ApiHostProvider : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 월드의 GameInstance에서 서비스별 호스트 설정 서브시스템을 반환하는 함수. */
	static UJWNU_GIS_ApiHostProvider* Get(const UObject* WorldContextObject);
	
	/** 서비스별 주소와 조회 성공 여부를 반환하는 함수. 빈 등록 주소는 true이며 미등록 서비스는 false와 TRASH_HOST를 반환한다. */
	bool GetHost(const EJWNU_ServiceType InServiceType, FString& OutHost) const;
	
	/** 서비스별 주소와 조회 성공 여부를 반환하는 함수. 빈 등록 주소는 true이며 미등록 서비스는 false와 TRASH_HOST를 반환한다. */
	UFUNCTION(BlueprintCallable, Category="JWNetworkUtility|Configuration")
	bool GetHost(const EJWNU_ServiceType InServiceType, EJWNU_HostGetResult& OutHostGetResult, FString& OutHost) const;

protected:

	/** 서비스 타입별 호스트 URL 매핑. */
	UPROPERTY(BlueprintReadOnly, Category = "JWNetworkUtility|Configuration")
	TMap<EJWNU_ServiceType, FString> ServiceTypeToHostMap;
};
