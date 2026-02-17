// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_BFL_AuthWidgetHelper.h"

#include "Styling/SlateColor.h"

void UJWNU_BFL_AuthWidgetHelper::OnRegisterEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult)
{
	const FString Trimmed = Input.ToString().TrimStartAndEnd();
	OutProcessed = FText::FromString(Trimmed);

	int32 AtIndex;
	if (Trimmed.FindChar(TEXT('@'), AtIndex) && AtIndex > 0)
	{
		const FString Domain = Trimmed.Mid(AtIndex + 1);
		if (Domain.Contains(TEXT(".")))
		{
			OutResult = EJWNU_RegisterEmailValidation::Satisfied;
			return;
		}
	}

	OutResult = EJWNU_RegisterEmailValidation::Unsatisfied;
}

void UJWNU_BFL_AuthWidgetHelper::OnRegisterCodeTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult)
{
	const FString Raw = Input.ToString();

	constexpr int32 MaxLen = 6;
	
	FString DigitsOnly;
	for (const TCHAR Ch : Raw)
	{
		if (DigitsOnly.Len() > MaxLen)
		{
			break;
		}
		if (FChar::IsDigit(Ch))
		{
			DigitsOnly.AppendChar(Ch);
		}
	}

	OutProcessed = FText::FromString(DigitsOnly);

	OutResult = (DigitsOnly.Len() == MaxLen)
		? EJWNU_RegisterEmailValidation::Satisfied
		: EJWNU_RegisterEmailValidation::Unsatisfied;
}

void UJWNU_BFL_AuthWidgetHelper::OnRegisterPrimaryPasswordTextBoxChanged(const FText& Input, EJWNU_RegisterPrimaryPasswordValidation& OutResult)
{
	const FString Password = Input.ToString();
	OutResult = IsPasswordFormatValid(Password)
		? EJWNU_RegisterPrimaryPasswordValidation::Satisfied
		: EJWNU_RegisterPrimaryPasswordValidation::Unsatisfied;
}

void UJWNU_BFL_AuthWidgetHelper::OnRegisterSecondaryPasswordTextBoxChanged(const FText& Input, const FText& FirstPassword, EJWNU_RegisterSecondaryPasswordValidation& OutResult)
{
	const FString First = FirstPassword.ToString();
	if (First.IsEmpty())
	{
		OutResult = EJWNU_RegisterSecondaryPasswordValidation::FirstPasswordEmpty;
		return;
	}
	
	const FString Second = Input.ToString();
	OutResult = (Second == First)
		? EJWNU_RegisterSecondaryPasswordValidation::Satisfied
		: EJWNU_RegisterSecondaryPasswordValidation::Unsatisfied;
}

void UJWNU_BFL_AuthWidgetHelper::OnLoginEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_LoginEmailValidation& OutResult)
{
	const FString Trimmed = Input.ToString().TrimStartAndEnd();
	OutProcessed = FText::FromString(Trimmed);

	int32 AtIndex;
	if (Trimmed.FindChar(TEXT('@'), AtIndex) && AtIndex > 0)
	{
		const FString Domain = Trimmed.Mid(AtIndex + 1);
		if (Domain.Contains(TEXT(".")))
		{
			OutResult = EJWNU_LoginEmailValidation::Satisfied;
			return;
		}
	}

	OutResult = EJWNU_LoginEmailValidation::Unsatisfied;
}

void UJWNU_BFL_AuthWidgetHelper::OnLoginPasswordTextBoxChanged(const FText& Input, EJWNU_LoginPasswordValidation& OutResult)
{
	const FString Password = Input.ToString();
	OutResult = (Password.Len() >= 10)
		? EJWNU_LoginPasswordValidation::Satisfied
		: EJWNU_LoginPasswordValidation::Unsatisfied;
}

FSlateColor UJWNU_BFL_AuthWidgetHelper::GetColorBySuccess(const bool bSuccess)
{
	const FSlateColor SuccessColor = { FLinearColor(0.0f, 1.0f, 0.0f, 1.0f) };
	const FSlateColor FailureColor = { FLinearColor(1.0f, 0.0f, 0.0f, 1.0f) };
	return bSuccess ? SuccessColor : FailureColor;
}

bool UJWNU_BFL_AuthWidgetHelper::IsPasswordFormatValid(const FString& Password)
{
	if (Password.Len() < 10)
	{
		return false;
	}

	bool bHasUpper = false;
	bool bHasLower = false;
	bool bHasDigit = false;
	bool bHasSpecial = false;

	for (const TCHAR Ch : Password)
	{
		if (FChar::IsUpper(Ch))       { bHasUpper = true; }
		else if (FChar::IsLower(Ch))  { bHasLower = true; }
		else if (FChar::IsDigit(Ch))  { bHasDigit = true; }
		else                          { bHasSpecial = true; }
	}

	return bHasUpper && bHasLower && bHasDigit && bHasSpecial;
}
