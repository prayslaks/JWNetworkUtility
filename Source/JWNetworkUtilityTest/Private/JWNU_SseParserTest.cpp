// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_SseParser.h"
#include "Misc/AutomationTest.h"
#include "Containers/StringConv.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_SseParserTest, "JWNetworkUtility.SSE.Parser", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_SseParserTest::RunTest(const FString& Parameters)
{
	const FString Wire = FString::Chr(0xfeff) + TEXT(": heartbeat\r\nid: a\r\nretry: 1200\r\nevent: delta\r\ndata: 안녕 ✈\r\ndata: second\r\n\r\nunknown: ignored\ndata:\n\nid:\n\nevent: reset\n\ndata: last\n\ndata: unfinished");
	const FTCHARToUTF8 Utf8(*Wire);
	// 가능한 모든 두 조각 경계와 바이트 단위 전송을 모두 검사한다.
	for (int32 Split = 0; Split <= Utf8.Length(); ++Split)
	{
		FJWNU_SseParser Parser;
		TArray<FJWNU_SseEvent> Events;
		TestTrue(TEXT("First fragment"), Parser.Feed(reinterpret_cast<const uint8*>(Utf8.Get()), Split, Events));
		TestTrue(TEXT("Second fragment"), Parser.Feed(reinterpret_cast<const uint8*>(Utf8.Get()) + Split, Utf8.Length() - Split, Events));
		Parser.Finish();
		if (!TestEqual(TEXT("Only terminated data events"), Events.Num(), 3)) { return false; }
		TestEqual(TEXT("Multiline unicode"), Events[0].Data, FString(TEXT("안녕 ✈\nsecond")));
		TestEqual(TEXT("Event type"), Events[0].EventType, FString(TEXT("delta")));
		TestEqual(TEXT("Retry hint"), Events[0].RetryMilliseconds, int64(1200));
		TestEqual(TEXT("Persistent id"), Events[1].Id, FString(TEXT("a")));
		TestEqual(TEXT("Empty data dispatches"), Events[1].Data, FString());
		TestEqual(TEXT("Id reset"), Events[2].Id, FString());
		TestEqual(TEXT("Type resets even without data"), Events[2].EventType, FString(TEXT("message")));
	}
	FJWNU_SseParser Parser;
	TArray<FJWNU_SseEvent> Events;
	for (int32 Index = 0; Index < Utf8.Length(); ++Index) { TestTrue(TEXT("Single byte"), Parser.Feed(reinterpret_cast<const uint8*>(Utf8.Get()) + Index, 1, Events)); }
	TestEqual(TEXT("Single byte event count"), Events.Num(), 3);
	Parser.Reset(8); Events.Reset();
	const uint8 TooLong[] = {'d','a','t','a',':','1','2','3','4'};
	TestFalse(TEXT("Line bound"), Parser.Feed(TooLong, UE_ARRAY_COUNT(TooLong), Events));
	Parser.Reset(16);
	const FTCHARToUTF8 LongEvent(TEXT("data: 1234\ndata: 5678\n\n"));
	TestFalse(TEXT("Event bound"), Parser.Feed(reinterpret_cast<const uint8*>(LongEvent.Get()), LongEvent.Length(), Events));
	Parser.Reset(); Events.Reset();
	const uint8 InvalidId[] = {'i','d',':','x',0,'y','\n','r','e','t','r','y',':','-','1','\n','\n','d','a','t','a',':','x','\n','\n'};
	Parser.Feed(InvalidId, UE_ARRAY_COUNT(InvalidId), Events);
	TestEqual(TEXT("Null id ignored"), Events[0].Id, FString());
	TestEqual(TEXT("Invalid retry ignored"), Events[0].RetryMilliseconds, int64(-1));
	return true;
}
#endif
