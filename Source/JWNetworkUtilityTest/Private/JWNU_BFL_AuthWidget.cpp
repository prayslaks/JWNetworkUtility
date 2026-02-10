// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_BFL_AuthWidget.h"

void UJWNU_BFL_AuthWidget::OnRegisterEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult)
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

void UJWNU_BFL_AuthWidget::OnRegisterCodeTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult)
{
	const FString Raw = Input.ToString();

	constexpr int32 MaxLen = 6;
	
	FString DigitsOnly;
	for (const TCHAR Ch : Raw)
	{
		if (DigitsOnly.Len() >= MaxLen)
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

void UJWNU_BFL_AuthWidget::OnRegisterPasswordFirstTextBoxChanged(const FText& Input, EJWNU_RegisterFirstPasswordValidation& OutResult)
{
	const FString Password = Input.ToString();
	OutResult = IsPasswordFormatValid(Password)
		? EJWNU_RegisterFirstPasswordValidation::Satisfied
		: EJWNU_RegisterFirstPasswordValidation::Unsatisfied;
}

void UJWNU_BFL_AuthWidget::OnRegisterPasswordSecondTextBoxChanged(const FText& Input, const FText& FirstPassword, EJWNU_RegisterSecondPasswordValidation& OutResult)
{
	const FString First = FirstPassword.ToString();
	if (First.IsEmpty())
	{
		OutResult = EJWNU_RegisterSecondPasswordValidation::FirstPasswordEmpty;
		return;
	}
	
	const FString Second = Input.ToString();
	OutResult = (Second == First)
		? EJWNU_RegisterSecondPasswordValidation::Satisfied
		: EJWNU_RegisterSecondPasswordValidation::Unsatisfied;
}

void UJWNU_BFL_AuthWidget::OnLoginEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_LoginEmailValidation& OutResult)
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

void UJWNU_BFL_AuthWidget::OnLoginPasswordTextBoxChanged(const FText& Input, EJWNU_LoginPasswordValidation& OutResult)
{
	const FString Password = Input.ToString();
	OutResult = (Password.Len() >= 10)
		? EJWNU_LoginPasswordValidation::Satisfied
		: EJWNU_LoginPasswordValidation::Unsatisfied;
}

bool UJWNU_BFL_AuthWidget::IsPasswordFormatValid(const FString& Password)
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
