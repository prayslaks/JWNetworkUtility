// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWNU_AIProviderKeysCustomizationHelpers.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_AIProviderKeysEditorTest,
	"JWNetworkUtility.AI.Editor.ExpiryInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_AIProviderKeysEditorTest::RunTest(const FString& Parameters)
{
	FDateTime Date;
	TestTrue(TEXT("Empty expiry is optional"), JWNU_AIKeyEditor::ParseExpiry(TEXT(""), Date) && Date.GetTicks() == 0);
	TestTrue(TEXT("Leap day accepted"), JWNU_AIKeyEditor::ParseExpiry(TEXT("2028-02-29"), Date));
	TestEqual(TEXT("Expiry includes whole UTC day"), Date.GetHour(), 23);
	TestEqual(TEXT("Expiry includes last second"), Date.GetSecond(), 59);
	TestFalse(TEXT("Invalid calendar date rejected"), JWNU_AIKeyEditor::ParseExpiry(TEXT("2027-02-29"), Date));
	TestFalse(TEXT("Locale-specific date rejected"), JWNU_AIKeyEditor::ParseExpiry(TEXT("09/27/2026"), Date));
	TestFalse(TEXT("Trailing date text rejected"), JWNU_AIKeyEditor::ParseExpiry(TEXT("2026-09-27-extra"), Date));
	TestFalse(TEXT("Non-numeric date rejected"), JWNU_AIKeyEditor::ParseExpiry(TEXT("2026-xx-27"), Date));
	return true;
}

#endif
