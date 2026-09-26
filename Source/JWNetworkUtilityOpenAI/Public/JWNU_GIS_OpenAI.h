// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "JWNU_GIS_OpenAI.generated.h"
class UJWNU_OpenAILiveSession;
class UJWNU_OpenAITranscriptionSession;
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
    friend class UJWNU_OpenAITranscriptionSession;
    bool Track(UJWNU_OpenAILiveSession* Session);
    bool Track(UJWNU_OpenAITranscriptionSession* Session);
    bool Tick(float DeltaSeconds);
    void WorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    UPROPERTY(Transient) TArray<TObjectPtr<UJWNU_OpenAILiveSession>> Active;
    UPROPERTY(Transient) TArray<TObjectPtr<UJWNU_OpenAITranscriptionSession>> ActiveTranscriptions;
    FTSTicker::FDelegateHandle Ticker;
    FDelegateHandle Cleanup;
    bool bStopping = false;
};
