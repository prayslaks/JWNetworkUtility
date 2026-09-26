// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_AudioTestWidget.h"
#include "JWNU_AudioTestActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

TSharedRef<SWidget> UJWNU_AudioTestWidget::RebuildWidget()
{
    SetIsFocusable(true);
    if (!WidgetTree) { WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree")); }
    if (!WidgetTree->RootWidget)
    {
        auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
        WidgetTree->RootWidget = Canvas;
        auto* Border = WidgetTree->ConstructWidget<UBorder>();
        Border->SetPadding(FMargin(20));
        Border->SetBrushColor(FLinearColor(.035f, .045f, .06f, .97f));
        auto* PanelSlot = Canvas->AddChildToCanvas(Border);
        PanelSlot->SetPosition(FVector2D(32, 32)); PanelSlot->SetSize(FVector2D(430, 360));
        auto* Box = WidgetTree->ConstructWidget<UVerticalBox>(); Border->SetContent(Box);
        auto* Title = WidgetTree->ConstructWidget<UTextBlock>();
        Title->SetText(FText::FromString(TEXT("JWNU 로컬 음성 테스트")));
        Box->AddChildToVerticalBox(Title);
        Status = WidgetTree->ConstructWidget<UTextBlock>(); Box->AddChildToVerticalBox(Status);
        Level = WidgetTree->ConstructWidget<UProgressBar>(); Box->AddChildToVerticalBox(Level);
        const TCHAR* Labels[] = { TEXT("녹음 시작 (최대 10초)"), TEXT("녹음 정지"), TEXT("녹음 재생"), TEXT("재생 정지") };
        TArray<UButton*> Buttons;
        for (const TCHAR* Label : Labels)
        {
            auto* Button = WidgetTree->ConstructWidget<UButton>();
            auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
            Text->SetText(FText::FromString(Label)); Button->AddChild(Text);
            Box->AddChildToVerticalBox(Button); Buttons.Add(Button);
        }
        RecordButton = Buttons[0]; StopRecordButton = Buttons[1]; PlayButton = Buttons[2]; StopPlayButton = Buttons[3];
        RecordButton->OnClicked.AddDynamic(this, &UJWNU_AudioTestWidget::StartRecording);
        StopRecordButton->OnClicked.AddDynamic(this, &UJWNU_AudioTestWidget::StopRecording);
        PlayButton->OnClicked.AddDynamic(this, &UJWNU_AudioTestWidget::PlayRecording);
        StopPlayButton->OnClicked.AddDynamic(this, &UJWNU_AudioTestWidget::StopPlayback);
        ErrorText = WidgetTree->ConstructWidget<UTextBlock>();
        ErrorText->SetAutoWrapText(true); ErrorText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, .35f, .25f)));
        Box->AddChildToVerticalBox(ErrorText);
    }
    Refresh();
    return Super::RebuildWidget();
}
void UJWNU_AudioTestWidget::Refresh()
{
    if (!Status) { return; }
    const bool bValid = IsValid(TestActor);
    const auto State = bValid ? TestActor->GetTestState() : EJWNU_AudioTestState::Idle;
    RecordButton->SetIsEnabled(bValid && State == EJWNU_AudioTestState::Idle);
    StopRecordButton->SetIsEnabled(bValid && State == EJWNU_AudioTestState::Recording);
    PlayButton->SetIsEnabled(bValid && State == EJWNU_AudioTestState::Idle && TestActor->GetRecordedSeconds() > 0);
    StopPlayButton->SetIsEnabled(bValid && State == EJWNU_AudioTestState::Playing);
    const TCHAR* Name = State == EJWNU_AudioTestState::Recording ? TEXT("녹음 중") : State == EJWNU_AudioTestState::Playing ? TEXT("재생 중") : TEXT("대기");
    Status->SetText(FText::FromString(bValid
        ? FString::Printf(TEXT("%s | 녹음 %.2f초 | 재생 %.2f초"), Name, TestActor->GetRecordedSeconds(), TestActor->GetPlaybackSeconds())
        : TEXT("Test Actor 참조를 지정하세요.")));
    const float RMS = bValid ? TestActor->GetInputLevel() : 0.f;
    // 작은 음성 신호도 읽기 쉽게 -60~0 dBFS를 막대의 0~1로 표시한다.
    Level->SetPercent(RMS > 0 ? FMath::Clamp((20.f * FMath::LogX(10.f, RMS) + 60.f) / 60.f, 0.f, 1.f) : 0.f);
    ErrorText->SetText(FText::FromString(bValid ? TestActor->GetLastError() : FString()));
}
void UJWNU_AudioTestWidget::NativeTick(const FGeometry& Geometry, float DeltaTime) { Super::NativeTick(Geometry, DeltaTime); Refresh(); }
void UJWNU_AudioTestWidget::StartRecording() { if (IsValid(TestActor)) { TestActor->StartRecording(); } Refresh(); }
void UJWNU_AudioTestWidget::StopRecording() { if (IsValid(TestActor)) { TestActor->StopRecording(); } Refresh(); }
void UJWNU_AudioTestWidget::PlayRecording() { if (IsValid(TestActor)) { TestActor->PlayRecording(); } Refresh(); }
void UJWNU_AudioTestWidget::StopPlayback() { if (IsValid(TestActor)) { TestActor->StopPlayback(); } Refresh(); }
void UJWNU_AudioTestWidget::NativeDestruct()
{
    if (IsValid(TestActor)) { TestActor->StopRecording(); TestActor->StopPlayback(); }
    Super::NativeDestruct();
}
