// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_WebSocketReceiveQueue.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_WebSocketQueueTest, "JWNetworkUtility.WebSocket.ReceiveQueue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_WebSocketQueueTest::RunTest(const FString& Parameters)
{
	FJWNU_WebSocketOptions Options;
	const TArray<uint8> Payload = {0, 1, 127, 128, 255, 0, 42};
	FString Unicode = TEXT("안녕 ✈🙂");
	Unicode.AppendChar(0); Unicode += TEXT("끝");
	const FTCHARToUTF8 Utf8(*Unicode, Unicode.Len());
	for (int32 Split = 0; Split <= Utf8.Length(); ++Split)
	{
		FJWNU_WebSocketReceiveQueue Queue(Options);
		Queue.PushTextFragment(Utf8.Get(), Split, false);
		TArray<FJWNU_WebSocketReceiveQueue::FItem> Messages;
		Queue.Take(Messages);
		TestTrue(TEXT("No partial UTF-8 delivery"), Messages.IsEmpty());
		Queue.PushTextFragment(Utf8.Get() + Split, Utf8.Length() - Split, true);
		TestTrue(TEXT("UTF-8 fragments within cap"), Queue.Take(Messages));
		if (TestEqual(TEXT("One assembled text"), Messages.Num(), 1))
		{
			TestTrue(TEXT("Every UTF-8 split preserves Unicode and embedded NUL"), Messages[0].Text == Unicode);
		}
	}
	for (int32 Split = 0; Split <= Payload.Num(); ++Split)
	{
		FJWNU_WebSocketReceiveQueue Queue(Options);
		Queue.PushBinary(Payload.GetData(), Split, false);
		TArray<FJWNU_WebSocketReceiveQueue::FItem> Items;
		TestTrue(TEXT("Partial frame within cap"), Queue.Take(Items));
		TestTrue(TEXT("No partial delivery"), Items.IsEmpty());
		Queue.PushBinary(Payload.GetData() + Split, Payload.Num() - Split, true);
		TestTrue(TEXT("Complete frame within cap"), Queue.Take(Items));
		if (TestEqual(TEXT("One binary message"), Items.Num(), 1))
		{
			TestTrue(TEXT("Exact bytes including null"), Items[0].Bytes == Payload);
		}
	}
	FJWNU_WebSocketReceiveQueue Empty(Options);
	Empty.PushBinary(nullptr, 0, true);
	TArray<FJWNU_WebSocketReceiveQueue::FItem> Items;
	Empty.Take(Items);
	TestEqual(TEXT("Empty binary message"), Items.Num(), 1);
	Options.MaxMessageBytes = 4;
	FJWNU_WebSocketReceiveQueue TooLarge(Options);
	TooLarge.PushBinary(Payload.GetData(), 3, false);
	TooLarge.PushBinary(Payload.GetData(), 2, true);
	TestFalse(TEXT("Partial binary cap"), TooLarge.Take(Items));
	Options.MaxQueuedBytes = 1;
	FJWNU_WebSocketReceiveQueue TooMany(Options);
	TooMany.PushBinary(nullptr, 0, true);
	TestFalse(TEXT("Queue includes empty message overhead"), TooMany.Take(Items));
	FJWNU_WebSocketReceiveQueue Stopped(FJWNU_WebSocketOptions{});
	Stopped.Stop(); Stopped.PushBinary(Payload.GetData(), Payload.Num(), true);
	Stopped.Take(Items);
	TestTrue(TEXT("Late callbacks ignored"), Items.IsEmpty());
	return true;
}
#endif
