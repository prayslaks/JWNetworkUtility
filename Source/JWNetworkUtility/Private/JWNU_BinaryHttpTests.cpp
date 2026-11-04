// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
#if WITH_DEV_AUTOMATION_TESTS
#include "JWNU_BinaryHttp.h"
#include "Async/Async.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_BinaryHttpTest, "JWNetworkUtility.BinaryHttp.Integration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_BinaryHttpTest::RunTest(const FString& Parameters)
{
	int32 Port = 0;
	if (!FParse::Value(FCommandLine::Get(), TEXT("JWNUBinaryTestPort="), Port) || Port <= 0 || Port > 65535) { AddWarning(TEXT("Skipped; run TestServer/run_binary_tests.py")); return true; }
	const auto Send = [Port](const FString& Path, int32 Limit, double Timeout, bool bCancelled)
	{
		return Async(EAsyncExecution::ThreadPool, [=]
		{
			TAtomic<bool> Cancelled(bCancelled);
			return FJWNU_BinaryHttp::Post(TEXT("127.0.0.1"), Path, Port, false, {{TEXT("X-Test-Key"), TEXT("fake-local-only")}}, TEXT("{\"fixture\":true}"), Timeout, Limit, Cancelled, {TEXT("X-Fixture")});
		}).Get();
	};
	auto Success = Send(TEXT("/binary"), 1024, 2, false);
	TestTrue(TEXT("Binary 0 and non UTF8 bytes preserved"), Success.Error.IsEmpty() && Success.StatusCode == 200 && Success.Bytes == TArray<uint8>{0,255,128,1});
	TestEqual(TEXT("Selected response header retained"), Success.Headers.FindRef(TEXT("x-fixture")), FString(TEXT("ok")));
	TestEqual(TEXT("Body limit enforced"), Send(TEXT("/binary"), 3, 2, false).Error, FString(TEXT("ResponseLimit")));
	TestEqual(TEXT("Redirect returned without following"), Send(TEXT("/redirect"), 1024, 2, false).StatusCode, 302);
	TestEqual(TEXT("Rate limit returned without retry"), Send(TEXT("/rate"), 1024, 2, false).StatusCode, 429);
	TestEqual(TEXT("Cancellation before send"), Send(TEXT("/cancelled"), 1024, 2, true).Error, FString(TEXT("Cancelled")));
	TestEqual(TEXT("Receive timeout"), Send(TEXT("/slow"), 1024, .1, false).Error, FString(TEXT("Timeout")));
	const auto Large = Async(EAsyncExecution::ThreadPool, [Port]
	{
		TAtomic<bool> Cancelled(false);
		return FJWNU_BinaryHttp::Post(TEXT("127.0.0.1"), TEXT("/large"), Port, false, {{TEXT("X-Test-Key"), TEXT("fake-local-only")}}, FString::ChrN(70000, 'x'), 2, 1024, Cancelled, {}, 100000);
	}).Get();
	TestTrue(TEXT("Explicit large request reaches server intact"), Large.Error.IsEmpty() && Large.StatusCode == 200);
    const auto Get = [Port](const FString& Path)
    {
        return Async(EAsyncExecution::ThreadPool, [Port, Path]
        {
            TAtomic<bool> Cancelled(false);
            return FJWNU_BinaryHttp::Get(TEXT("127.0.0.1"), Path, Port, false, {{TEXT("X-Test-Key"), TEXT("fake-local-only")}}, 2, 1024, Cancelled);
        }).Get();
    };
    const auto Read = Get(TEXT("/voices"));
    TestTrue(TEXT("GET preserves custom header and response bytes without request body"), Read.Error.IsEmpty() && Read.StatusCode == 200 && Read.Bytes == TArray<uint8>{0,255,128,1});
    TestEqual(TEXT("GET does not follow redirects"), Get(TEXT("/redirect-get")).StatusCode, 302);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_BinaryHttpLimitsTest, "JWNetworkUtility.BinaryHttp.Limits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_BinaryHttpLimitsTest::RunTest(const FString& Parameters)
{
	const auto Check = [](const FString& Body, int32 RequestLimit)
	{
		return Async(EAsyncExecution::ThreadPool, [Body, RequestLimit]
		{
			TAtomic<bool> Cancelled(true);
			return FJWNU_BinaryHttp::Post(TEXT("127.0.0.1"), TEXT("/never-sent"), 1, false, {}, Body, 2, 1024, Cancelled, {}, RequestLimit).Error;
		}).Get();
	};
	TestEqual(TEXT("Default cap stays 64KiB"), Check(FString::ChrN(70000, 'x'), 65536), FString(TEXT("InvalidRequest")));
	TestEqual(TEXT("UTF8 byte count enforced"), Check(FString::ChrN(30000, TCHAR(0xAC00)), 65536), FString(TEXT("RequestLimit")));
	TestEqual(TEXT("Hard request cap enforced"), Check(TEXT("{}"), 65 * 1024 * 1024), FString(TEXT("InvalidRequest")));
	return true;
}
#endif
