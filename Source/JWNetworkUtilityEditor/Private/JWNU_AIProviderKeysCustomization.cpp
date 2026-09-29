// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_AIProviderKeysCustomization.h"

#include "JWNU_AIProviderKeysCustomizationHelpers.h"

#include "JWNU_AIProviderSettings.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

/** 공급자별 저장 목록을 작업 완료 시에만 다시 읽는 관리 패널이다. */
class SJWNU_AIProviderKeysPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SJWNU_AIProviderKeysPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UJWNU_AIProviderSettings>, Settings)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		Settings = Args._Settings;
		ChildSlot [SAssignNew(Content, SVerticalBox)];
		Refresh();
	}

private:
	TWeakObjectPtr<UJWNU_AIProviderSettings> Settings;
	TSharedPtr<SVerticalBox> Content;
	FText Error;

	void Refresh()
	{
		using namespace JWNU_AIKeyEditor;
		Content->ClearChildren();
		Content->AddSlot().AutoHeight().Padding(0, 4, 0, 12)
		[
			SNew(STextBlock).AutoWrapText(true).Text(Text(TEXT("이 Windows 계정의 개인 키를 관리합니다. 저장한 키와 목록 정보는 OS 암호화로 보호됩니다.\n사용 키 변경은 다음 API 요청부터 적용됩니다. 런타임 임시 키는 각 GameInstance에서 따로 적용됩니다.")))
		];
		Content->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[
			SNew(SButton).HAlign(HAlign_Center).Text(Text(TEXT("목록 새로고침")))
			.OnClicked_Lambda([this]() { Error = FText(); Refresh(); return FReply::Handled(); })
		];
		Content->AddSlot().AutoHeight()
		[
			SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FLinearColor(1.f, .35f, .25f))
			.Text_Lambda([this]() { return Error; })
		];
		if (!Settings.IsValid()) return;
		for (const FJWNU_AIProviderDefinition& Definition : UJWNU_AIProviderSettings::GetProviderCatalog())
		{
			const FString Provider = Definition.Id;
			const bool bSupported = !Definition.bDeprecated;
			TArray<FJWNU_APIKeyInfo> Keys;
			const bool bReadable = Settings->GetSavedKeys(Provider, Keys);
			const bool bActive = Keys.ContainsByPredicate([](const FJWNU_APIKeyInfo& Key) { return Key.bActive; });
			const FString ProviderName = Provider + (bSupported ? TEXT("") : TEXT("  [지원 종료]"));
			const FString Status = !bReadable ? TEXT("저장소를 읽을 수 없습니다") : bActive ? TEXT("사용 키 선택됨") : TEXT("사용 키 없음");
			TSharedRef<SVerticalBox> Card = SNew(SVerticalBox);
			Content->AddSlot().AutoHeight().Padding(0, 8)
			[
				SNew(SBorder).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder")).Padding(16) [Card]
			];
			Card->AddSlot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
				[SNew(STextBlock).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle")).Text(Text(ProviderName))]
				+ SHorizontalBox::Slot().AutoWidth()
				[SNew(SButton).Text(Text(TEXT("+ 키 추가"))).IsEnabled(bSupported && bReadable && Keys.Num() < 32)
				.OnClicked_Lambda([this, Provider]() { OpenForm(Provider, FJWNU_APIKeyInfo()); return FReply::Handled(); })]
			];
			Card->AddSlot().AutoHeight().Padding(0, 8)
			[SNew(STextBlock).AutoWrapText(true).Text(Text(bReadable ? FString::Printf(TEXT("등록 %d / 32개  ·  %s"), Keys.Num(), *Status) : Status))];
			if (bReadable && Keys.IsEmpty())
				Card->AddSlot().AutoHeight().Padding(0, 8)[SNew(STextBlock).Text(Text(bSupported ? TEXT("등록된 키가 없습니다. 첫 키를 추가하면 자동으로 선택됩니다.") : TEXT("지원이 종료된 공급자입니다. 저장된 키만 확인하고 삭제할 수 있습니다."))).AutoWrapText(true)];
			for (const FJWNU_APIKeyInfo& Key : Keys)
			{
				const bool bExpired = Key.ExpiresUtc.GetTicks() && Key.ExpiresUtc < FDateTime::UtcNow();
				const FString Caption = Key.Label + TEXT("   ") + Key.MaskedKey + (Key.bActive ? TEXT("   [사용 키]") : TEXT("")) + (bExpired ? TEXT("   [관리 기한 경과]") : TEXT(""));
				const FString Created = Date(Key.CreatedUtc);
				Card->AddSlot().AutoHeight().Padding(0, 8)
				[
					SNew(SBorder).Padding(12).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Text(Caption)).AutoWrapText(true)]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
						[SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Text(Text(FString::Printf(TEXT("등록 %s  ·  수정 %s\n관리용 만료일 %s (UTC)"), *Created, *Date(Key.UpdatedUtc), *Date(Key.ExpiresUtc))))]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
							[SNew(SButton).Text(Text(TEXT("사용 키로 선택"))).IsEnabled(bSupported && !Key.bActive)
							.OnClicked_Lambda([this, Provider, Key]() { Complete(Settings.IsValid() && Settings->SelectManagedKey(Provider, Key.Id)); return FReply::Handled(); })]
							+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
							[SNew(SButton).Text(Text(TEXT("수정"))).IsEnabled(bSupported).OnClicked_Lambda([this, Provider, Key]() { OpenForm(Provider, Key); return FReply::Handled(); })]
							+ SHorizontalBox::Slot().AutoWidth()
							[SNew(SButton).Text(Text(TEXT("삭제"))).OnClicked_Lambda([this, Provider, Key]()
							{
								const FText Prompt = Text(Key.Label + TEXT(" 키를 이 PC의 목록에서 삭제할까요?\n공급자 사이트의 키는 폐기되지 않습니다.") + (Key.bActive ? TEXT("\n다른 키를 선택할 때까지 사용 키가 없는 상태가 됩니다.") : TEXT("")));
								if (FMessageDialog::Open(EAppMsgType::YesNo, Prompt) == EAppReturnType::Yes)
									Complete(Settings.IsValid() && Settings->DeleteManagedKey(Provider, Key.Id));
								return FReply::Handled();
							})]
						]
					]
				];
			}
		}
		Content->AddSlot().AutoHeight().Padding(0, 8)
		[SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		.Text(Text(TEXT("만료일은 직접 지정하는 관리 정보이며, 기한이 지나도 자동 차단하지 않습니다. 실제 유효성·사용량·권한은 공급자 사이트에서 확인하세요.")))];
	}

	void Complete(bool bSuccess)
	{
		Error = bSuccess ? FText() : JWNU_AIKeyEditor::Text(TEXT("작업에 실패했습니다. 저장소 접근 권한과 Windows 계정을 확인한 후 다시 시도하세요."));
		Refresh();
	}

	void OpenForm(FString Provider, const FJWNU_APIKeyInfo& Info)
	{
		using namespace JWNU_AIKeyEditor;
		const bool bEdit = Info.Id.IsValid();
		const TSharedRef<FJWNU_KeyForm> Form = MakeShared<FJWNU_KeyForm>();
		const TSharedRef<SWindow> Window = SNew(SWindow).Title(Text(bEdit ? TEXT("API 키 수정") : TEXT("API 키 추가")))
			.ClientSize(FVector2D(520, 330)).SupportsMinimize(false).SupportsMaximize(false).SizingRule(ESizingRule::Autosized);
		Form->Window = Window;
		const TWeakPtr<SJWNU_AIProviderKeysPanel> WeakPanel = SharedThis(this);
		Window->SetContent(
			SNew(SBorder).Padding(20)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Text(TEXT("이름")))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 12)
				[SAssignNew(Form->Label, SEditableTextBox).MinDesiredWidth(460).Text(Text(Info.Label)).HintText(Text(TEXT("예: 개발용, 테스트용")))]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Text(bEdit ? TEXT("새 API 키 · 비워두면 기존 값 유지") : TEXT("API 키")))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 12)
				[SAssignNew(Form->Key, SEditableTextBox).IsPassword(true).AllowContextMenu(false)]
				+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(Text(TEXT("관리용 만료일 · 선택 사항 · UTC")))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 12)
				[SAssignNew(Form->Expiry, SEditableTextBox).Text(Text(Info.ExpiresUtc.GetTicks() ? Date(Info.ExpiresUtc) : FString())).HintText(Text(TEXT("YYYY-MM-DD")))]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
				[SNew(STextBlock).WrapTextAt(460).ColorAndOpacity(FLinearColor(1.f, .35f, .25f)).Text_Lambda([Form]() { return Form->Error; })]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
					[SNew(SButton).Text(Text(TEXT("취소"))).OnClicked_Lambda([Form]() { if (const auto Dialog = Form->Window.Pin()) Dialog->RequestDestroyWindow(); return FReply::Handled(); })]
					+ SHorizontalBox::Slot().AutoWidth()
					[SNew(SButton).Text(Text(TEXT("암호화하여 저장"))).OnClicked_Lambda([Form, WeakPanel, Info, Provider, bEdit]()
					{
						const auto Panel = WeakPanel.Pin();
						if (!Panel || !Panel->Settings.IsValid()) return FReply::Handled();
						const FString Label = Form->Label->GetText().ToString().TrimStartAndEnd();
						FString Secret = Form->Key->GetText().ToString().TrimStartAndEnd();
						FDateTime Expiry;
						if (Label.IsEmpty() || Label.Len() > 64) Form->Error = Text(TEXT("이름은 1~64자로 입력하세요."));
						else if (!bEdit && Secret.IsEmpty()) Form->Error = Text(TEXT("등록할 API 키를 입력하세요."));
						else if (!ParseExpiry(Form->Expiry->GetText().ToString().TrimStartAndEnd(), Expiry)) Form->Error = Text(TEXT("만료일은 유효한 YYYY-MM-DD 날짜로 입력하세요."));
						else
						{
							FGuid SavedId;
							if (Panel->Settings->SaveManagedKey(Provider, Info.Id, Label, Secret, Expiry, SavedId))
							{
								Panel->Complete(true);
								if (const auto Dialog = Form->Window.Pin()) Dialog->RequestDestroyWindow();
							}
							else Form->Error = Text(TEXT("저장 실패: 저장소 접근 권한 또는 키 크기를 확인하세요."));
						}
						JWNU_AIKeyEditor::ClearSensitiveString(Secret);
						return FReply::Handled();
					})]
				]
			]);
		Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([Form](const TSharedRef<SWindow>&) { Form->Key->SetText(FText()); }));
		FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().FindBestParentWindowForDialogs(nullptr));
	}
};

TSharedRef<IDetailCustomization> FJWNU_AIProviderKeysCustomization::MakeInstance()
{
	return MakeShared<FJWNU_AIProviderKeysCustomization>();
}

void FJWNU_AIProviderKeysCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	if (Objects.Num() != 1) return;
	UJWNU_AIProviderSettings* Settings = Cast<UJWNU_AIProviderSettings>(Objects[0].Get());
	if (!Settings) return;
	FString SearchTerms = TEXT("API Keys 공급자 키");
	for (const FJWNU_AIProviderDefinition& Definition : UJWNU_AIProviderSettings::GetProviderCatalog())
	{
		SearchTerms += TEXT(" ") + Definition.Id;
	}
	DetailBuilder.EditCategory(TEXT("API Key Manager"), JWNU_AIKeyEditor::Text(TEXT("API 키 관리")), ECategoryPriority::Important)
		.AddCustomRow(JWNU_AIKeyEditor::Text(*SearchTerms)).WholeRowContent()
		[SNew(SJWNU_AIProviderKeysPanel).Settings(Settings)];
}
