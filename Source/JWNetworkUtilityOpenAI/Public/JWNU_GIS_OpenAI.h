// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "JWNU_GIS_OpenAI.generated.h"
class UJWNU_OpenAILiveSession;
class UJWNU_OpenAITranscriptionSession;
class UJWNU_GptLiveTranscriptor;
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
    friend class UJWNU_GptLiveTranscriptor;
    bool Track(UJWNU_OpenAILiveSession* Session);
    bool Track(UJWNU_OpenAITranscriptionSession* Session);
    bool Track(UJWNU_GptLiveTranscriptor* Transcriptor);
    bool Tick(float DeltaSeconds);
    void WorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    /** 활성 음성 대화 세션의 GC 수명과 시간 제한·월드 종료 처리를 위한 보관 목록 필드. */
    UPROPERTY(Transient) TArray<TObjectPtr<UJWNU_OpenAILiveSession>> Active;
    /** 활성 원시 전사 세션의 GC 수명과 시간 제한·월드 종료 처리를 위한 보관 목록 필드. */
    UPROPERTY(Transient) TArray<TObjectPtr<UJWNU_OpenAITranscriptionSession>> ActiveTranscriptions;
    /** 활성 누적 자막 전사기의 GC 수명과 commit/final 시간 제한 처리를 위한 보관 목록 필드. */
    UPROPERTY(Transient) TArray<TObjectPtr<UJWNU_GptLiveTranscriptor>> ActiveTranscriptors;
    FTSTicker::FDelegateHandle Ticker;
    FDelegateHandle Cleanup;
    bool bStopping = false;
};
