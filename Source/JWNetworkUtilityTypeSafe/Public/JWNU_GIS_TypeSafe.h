// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "JWNU_GIS_TypeSafe.generated.h"

class UJWNU_TypeSafeRequest;
class UWorld;

/** 진행 중인 TypeSafe 요청의 GC·실시간 Pump·월드 종료를 관리한다. */
UCLASS()
class JWNETWORKUTILITYTYPESAFE_API UJWNU_GIS_TypeSafe : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
private:
    friend class UJWNU_TypeSafeRequest;
    bool Track(UJWNU_TypeSafeRequest* Request);
    bool Tick(float DeltaSeconds);
    void WorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    /** 종료 전 요청을 강하게 보관하는 필드. */
    UPROPERTY(Transient) TArray<TObjectPtr<UJWNU_TypeSafeRequest>> Active;
    FTSTicker::FDelegateHandle Ticker;
    FDelegateHandle Cleanup;
    bool bStopping = false;
};
