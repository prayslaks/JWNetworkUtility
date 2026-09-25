// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|Audio Test", meta=(ExposeOnSpawn=true)) TObjectPtr<AJWNU_AudioTestActor> TestActor;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void NativeDestruct() override;
private:
    UFUNCTION() void StartRecording();
    UFUNCTION() void StopRecording();
    UFUNCTION() void PlayRecording();
    UFUNCTION() void StopPlayback();
    void Refresh();
    UPROPERTY(Transient) TObjectPtr<UButton> RecordButton;
    UPROPERTY(Transient) TObjectPtr<UButton> StopRecordButton;
    UPROPERTY(Transient) TObjectPtr<UButton> PlayButton;
    UPROPERTY(Transient) TObjectPtr<UButton> StopPlayButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ErrorText;
    UPROPERTY(Transient) TObjectPtr<UProgressBar> Level;
};
