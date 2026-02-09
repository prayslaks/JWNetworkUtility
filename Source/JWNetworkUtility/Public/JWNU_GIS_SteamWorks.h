// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWNU_GIS_SteamWorks.generated.h"

/**
 * 클래스 전용 로그 카테고리 선언
 */
DECLARE_LOG_CATEGORY_EXTERN(JW_GIS_SteamWorks, Log, All);

/**
 * 스팀 인증 작업을 처리하는 게임인스턴스 서브시스템 클래스.
 */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_SteamWorks : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
