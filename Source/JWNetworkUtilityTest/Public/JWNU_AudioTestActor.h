// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JWNU_AudioTypes.h"
#include "JWNU_AudioTestActor.generated.h"
class UJWNU_MicrophoneCaptureComponent;
class UJWNU_PCMPlayerComponent;
class UJWNU_AudioTestWidget;
class APlayerController;

/** 테스트 액터의 대기·마이크 녹음·저장 PCM 재생 단계를 구분한다. */
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
    /** 녹음과 재생 버튼의 사용 가능 여부를 판단할 현재 테스트 상태를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") EJWNU_AudioTestState GetTestState() const { return State; }
    /** 저장된 PCM 표본 수와 표본율로 녹음 길이(초)를 계산하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") float GetRecordedSeconds() const;
    /** 재생 큐 소비량으로 재생 위치(초)를 반환하는 함수. 정지하면 마지막 위치를 보존한다. */
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") float GetPlaybackSeconds() const;
    /** 마이크의 최근 PCM 청크 RMS 입력 레벨을 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") float GetInputLevel() const;
    /** 가장 최근 오디오 실패의 표시용 메시지를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|Audio Test") FString GetLastError() const { return LastError; }
    /** 다음 녹음에서 생성할 PCM의 표본율(Hz) 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|Audio Test") int32 RecordingSampleRate = 24000;
    /** 녹음에 사용할 마이크 인덱스 필드. -1이면 기본 입력 장치다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|Audio Test") int32 MicrophoneDeviceIndex = -1;
    /** 자동으로 녹음을 멈출 최대 길이(초) 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|Audio Test", meta=(ClampMin="0.1", ClampMax="10")) float MaxRecordingSeconds = 10.f;
    /** 로컬 입력 장치에서 PCM을 수집하는 캡처 컴포넌트 필드. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JWNU|Audio Test") TObjectPtr<UJWNU_MicrophoneCaptureComponent> Microphone;
    /** 저장한 PCM을 청크로 공급받아 재생하는 컴포넌트 필드. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="JWNU|Audio Test") TObjectPtr<UJWNU_PCMPlayerComponent> Player;
    /** 녹음·재생을 중단한 오류와 표시할 진단 정보를 전달하는 이벤트 필드. */
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
    /** ShowTestPanel로 생성해 표시 중인 테스트 위젯을 보관하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_AudioTestWidget> TestPanel;
};
