// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JWNU_AudioTestWidget.generated.h"
class AJWNU_AudioTestActor;
class UButton;
class UTextBlock;
class UProgressBar;

/** 기본 녹음·재생 버튼과 상태 표시를 제공하는 UMG 테스트 패널이다. */
UCLASS(Blueprintable)
class JWNETWORKUTILITYTEST_API UJWNU_AudioTestWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    /** 버튼 입력을 전달하고 녹음·재생 상태를 조회할 테스트 액터 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|Audio Test", meta=(ExposeOnSpawn=true)) TObjectPtr<AJWNU_AudioTestActor> TestActor;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void NativeDestruct() override;
private:
    /** 녹음 버튼 입력을 테스트 액터의 StartRecording에 전달하는 함수. */
    UFUNCTION() void StartRecording();
    /** 녹음 정지 버튼 입력을 테스트 액터에 전달하는 함수. */
    UFUNCTION() void StopRecording();
    /** 재생 버튼 입력을 테스트 액터의 PlayRecording에 전달하는 함수. */
    UFUNCTION() void PlayRecording();
    /** 재생 정지 버튼 입력을 테스트 액터에 전달하는 함수. */
    UFUNCTION() void StopPlayback();
    void Refresh();
    /** 새 녹음을 시작하는 버튼을 보관하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UButton> RecordButton;
    /** 현재 녹음을 멈추는 버튼을 보관하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UButton> StopRecordButton;
    /** 저장된 녹음을 재생하는 버튼을 보관하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UButton> PlayButton;
    /** 현재 재생을 중단하는 버튼을 보관하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UButton> StopPlayButton;
    /** 녹음·재생 상태와 길이를 표시하는 텍스트 필드. */
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
    /** 최근 오디오 오류 메시지를 표시하는 텍스트 필드. */
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ErrorText;
    /** 실시간 마이크 입력 레벨을 표시하는 진행 막대 필드. */
    UPROPERTY(Transient) TObjectPtr<UProgressBar> Level;
};
