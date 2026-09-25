// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JWNU_AudioTypes.h"
#include "JWNU_MicrophoneCaptureComponent.generated.h"
struct FJWNU_MicrophoneCapture;

/** 로컬 마이크를 PCM16 mono 청크로 변환한다. 네트워크와 독립적이다. */
UCLASS(ClassGroup=(JWNU), meta=(BlueprintSpawnableComponent))
class JWNETWORKUTILITYAUDIO_API UJWNU_MicrophoneCaptureComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UJWNU_MicrophoneCaptureComponent();
    /** 기본 장치 또는 지정 인덱스에서 캡처를 시작하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio")
    bool StartCapture(int32 SampleRate = 24000, int32 DeviceIndex = -1);
    /** 현재 마이크 목록을 조회하는 함수. 녹음을 시작하지 않는다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio") static TArray<FJWNU_AudioCaptureDevice> GetCaptureDevices();
    /** 녹음을 중단하고 미전송 버퍼를 버리는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio") void StopCapture();
    UFUNCTION(BlueprintPure, Category="JWNU|Audio") bool IsCapturing() const;
    /** 최근 PCM 청크의 RMS 음량을 0~1로 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|Audio") float GetInputLevel() const { return InputLevel; }
    UPROPERTY(BlueprintAssignable, Category="JWNU|Audio") FJWNU_PCMReceivedBP OnPCM;
    UPROPERTY(BlueprintAssignable, Category="JWNU|Audio") FJWNU_AudioErrorBP OnError;
    FJWNU_PCMReceivedNative OnPCMNative;
    FJWNU_AudioErrorNative OnErrorNative;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void BeginDestroy() override;
private:
    void Report(const FString& Message);
    TSharedPtr<FJWNU_MicrophoneCapture> Capture;
    float InputLevel = 0;
    uint64 CaptureGeneration = 0;
};
