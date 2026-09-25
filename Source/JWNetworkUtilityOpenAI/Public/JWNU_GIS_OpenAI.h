// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "JWNU_GIS_OpenAI.generated.h"
class UJWNU_OpenAILiveSession;
class UWorld;

/** 활성 세션의 GC 수명·타임아웃·월드 정리를 관리한다. */
UCLASS()
class JWNETWORKUTILITYOPENAI_API UJWNU_GIS_OpenAI : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
private:
    friend class UJWNU_OpenAILiveSession;
    bool Track(UJWNU_OpenAILiveSession* Session);
    bool Tick(float DeltaSeconds);
    void WorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    UPROPERTY(Transient) TArray<TObjectPtr<UJWNU_OpenAILiveSession>> Active;
    FTSTicker::FDelegateHandle Ticker;
    FDelegateHandle Cleanup;
    bool bStopping = false;
};
