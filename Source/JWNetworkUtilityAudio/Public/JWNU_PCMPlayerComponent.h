// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JWNU_AudioTypes.h"
#include "JWNU_PCMPlayerComponent.generated.h"
class UAudioComponent;
class USoundWaveProcedural;

/** PCM16 mono 출력의 로컬 재생 큐를 소유한다. 네트워크와 독립적이다. */
UCLASS(ClassGroup=(JWNU), meta=(BlueprintSpawnableComponent))
class JWNETWORKUTILITYAUDIO_API UJWNU_PCMPlayerComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** PCM 재생기를 준비하는 함수. 실제 재생은 첫 QueuePCM에서 시작한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio") bool StartPlayer(int32 SampleRate = 24000);
    /** PCM을 순서대로 추가하는 함수. 2초를 넘는 적체는 false와 OnError로 알린다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio") bool QueuePCM(const TArray<uint8>& PCM16);
    /** 남은 출력까지 즉시 중단하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio") void StopPlayer();
    UFUNCTION(BlueprintPure, Category="JWNU|Audio") float GetBufferedSeconds() const;
    UPROPERTY(BlueprintAssignable, Category="JWNU|Audio") FJWNU_AudioErrorBP OnError;
    FJWNU_AudioErrorNative OnErrorNative;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
private:
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Output;
    UPROPERTY(Transient) TObjectPtr<USoundWaveProcedural> Wave;
    int32 Rate = 24000;
};
