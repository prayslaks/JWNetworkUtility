// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SWindow.h"

namespace JWNU_AIKeyEditor
{
	inline void ClearSensitiveString(FString& Value)
	{
		volatile TCHAR* Data = Value.GetCharArray().GetData();
		for (int32 Index = 0; Index < Value.GetCharArray().Num(); ++Index) Data[Index] = 0;
		Value.Empty();
	}

	inline FText Text(const FString& Value) { return FText::FromString(Value); }
	inline FString Date(const FDateTime& Value)
	{
		return Value.GetTicks() ? Value.ToString(TEXT("%Y-%m-%d")) : TEXT("미지정");
	}

	/** 날짜 입력은 UTC 기준 하루의 끝으로 저장한다. */
	inline bool ParseExpiry(const FString& Input, FDateTime& OutDate)
	{
		OutDate = FDateTime();
		if (Input.IsEmpty()) return true;
		if (Input.Len() != 10 || Input[4] != TEXT('-') || Input[7] != TEXT('-')) return false;
		for (int32 Index = 0; Index < Input.Len(); ++Index)
			if (Index != 4 && Index != 7 && !FChar::IsDigit(Input[Index])) return false;
		return FDateTime::ParseIso8601(*(Input + TEXT("T23:59:59Z")), OutDate) && Date(OutDate) == Input;
	}

	/** 대화상자의 입력만 보유하며 저장된 비밀값은 미리 채우지 않는다. */
	struct FJWNU_KeyForm
	{
		TSharedPtr<SEditableTextBox> Label, Key, Expiry;
		TWeakPtr<SWindow> Window;
		FText Error;
	};
}
