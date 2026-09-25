// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JWNU_AudioTypes.h"
#include "JWNU_AudioTestActor.generated.h"
class UJWNU_MicrophoneCaptureComponent;
class UJWNU_PCMPlayerComponent;
class UJWNU_AudioTestWidget;
class APlayerController;

UENUM(BlueprintType)
enum class EJWNU_AudioTestState : uint8 { Idle, Recording, Playing };

/** 로컬 녹음 버퍼와 청크 재생을 제공하는 UMG 테스트용 Actor다. 네트워크를 사용하지 않는다. */
UCLASS(Blueprintable)
class JWNETWORKUTILITYTEST_API AJWNU_AudioTestActor : public AActor
{
    GENERATED_BODY()
public:
    AJWNU_AudioTestActor();
    /** 기존 녹음을 지우고 마이크 녹음을 시작하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio Test") bool StartRecording();
    /** 녹음을 멈추고 수집된 PCM을 보존하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio Test") void StopRecording();
    /** 저장된 PCM을 처음부터 재생하는 함수. 재생 중 호출은 거절한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio Test") bool PlayRecording();
    /** 재생을 즉시 멈추고 녹음은 보존하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio Test") void StopPlayback();
    /** 외부·합성 PCM을 녹음 버퍼로 가져오는 함수. Idle에서만 허용한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio Test") bool LoadRecordingPCM(const TArray<uint8>& PCM16, int32 SampleRate);
    /** 기본 UMG 패널을 생성하고 화면에 추가하는 함수. 입력 모드는 호출자가 설정한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio Test") UJWNU_AudioTestWidget* ShowTestPanel(APlayerController* PlayerController);
    /** 이 Actor가 생성한 패널을 제거하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Audio Test") void HideTestPanel();
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") EJWNU_AudioTestState GetTestState() const { return State; }
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") float GetRecordedSeconds() const;
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") float GetPlaybackSeconds() const;
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") float GetInputLevel() const;
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") FString GetLastError() const { return LastError; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|Audio Test") int32 RecordingSampleRate = 24000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|Audio Test") int32 MicrophoneDeviceIndex = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|Audio Test", meta=(ClampMin="0.1", ClampMax="10")) float MaxRecordingSeconds = 10.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JWNU|Audio Test") TObjectPtr<UJWNU_MicrophoneCaptureComponent> Microphone;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JWNU|Audio Test") TObjectPtr<UJWNU_PCMPlayerComponent> Player;
    UPROPERTY(BlueprintAssignable, Category="JWNU|Audio Test") FJWNU_AudioErrorBP OnError;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FJWNU_AudioWorkflowTest;
#endif
    void ReceivePCM(const TArray<uint8>& Bytes);
    void AudioError(const FJWNU_AudioError& Error);
    void Report(const FString& Message);
    void FeedPlayback();
    TArray<uint8> Recording;
    int32 RecordedRate = 24000;
    int32 RecordingLimitBytes = 0;
    int32 PlaybackOffset = 0;
    float StoppedPlaybackSeconds = 0;
    EJWNU_AudioTestState State = EJWNU_AudioTestState::Idle;
    FString LastError;
    bool bEnding = false;
    bool bDispatchingError = false;
    double LastPlaybackProgress = 0;
    float LastQueuedSeconds = 0;
    UPROPERTY(Transient) TObjectPtr<UJWNU_AudioTestWidget> TestPanel;
};
