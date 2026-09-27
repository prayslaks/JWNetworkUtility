// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

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
    /** 마이크 캡처가 현재 실행 중인지 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|Audio") bool IsCapturing() const;
    /** 최근 PCM 청크의 RMS 음량을 0~1로 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|Audio") float GetInputLevel() const { return InputLevel; }
    /** 지정 표본율의 PCM16 LE mono 청크를 게임 스레드에 전달하는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|Audio") FJWNU_PCMReceivedBP OnPCM;
    /** 캡처 장치·입력 버퍼 오류를 알리는 이벤트 필드. */
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
