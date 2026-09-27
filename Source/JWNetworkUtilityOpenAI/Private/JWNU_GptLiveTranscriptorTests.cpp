// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWNU_GptLiveTranscriptor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_GptLiveStreamingTest,
	"JWNetworkUtility.OpenAI.Transcription.GptLiveTranscriptor.StreamingResults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_GptLiveStreamingTest::RunTest(const FString& Parameters)
{
	UJWNU_GptLiveTranscriptor* Gpt = NewObject<UJWNU_GptLiveTranscriptor>();
	Gpt->bReady = true;
	TArray<FJWNU_GptLiveTranscript> Results;
	Gpt->OnTranscriptNative.AddLambda([&Results](const FJWNU_GptLiveTranscript& Value) { Results.Add(Value); });
	FJWNU_OpenAITranscript Update;
	Update.ItemId = TEXT("first"); Update.Delta = TEXT("자유"); Gpt->Transcript(Update);
	Update.Delta = TEXT(" 전투"); Gpt->Transcript(Update);
	TestEqual(TEXT("Deltas arrive before commit acknowledgement"), Results.Num(), 2);
	TestEqual(TEXT("Partial output is accumulated text"), Results.Last().Text, FString(TEXT("자유 전투")));
	TestEqual(TEXT("First delta retains original chunk"), Results[0].Delta, FString(TEXT("자유")));
	TestEqual(TEXT("Second delta is not accumulated and keeps whitespace"), Results[1].Delta, FString(TEXT(" 전투")));
	Gpt->AwaitingCommits.Add(FPlatformTime::Seconds() + 60);
	FJWNU_OpenAITranscriptionCommit Commit; Commit.ItemId = TEXT("first"); Gpt->Committed(Commit);
	Gpt->AwaitingCommits.Add(FPlatformTime::Seconds() + 60);
	Commit.PreviousItemId = TEXT("first"); Commit.ItemId = TEXT("second"); Gpt->Committed(Commit);
	Update.ItemId = TEXT("second"); Update.bFinal = true; Update.Transcript = TEXT("편대 비행"); Gpt->Transcript(Update);
	TestEqual(TEXT("Later final waits for preceding turn"), Results.Num(), 2);
	Update.ItemId = TEXT("first"); Update.Transcript = TEXT("자유 전투 하지 마"); Gpt->Transcript(Update);
	TestEqual(TEXT("Both finals flush in input order"), Results.Num(), 4);
	TestEqual(TEXT("Final replaces provisional text"), Results[2].Text, FString(TEXT("자유 전투 하지 마")));
	TestEqual(TEXT("Second final retains identity"), Results[3].UtteranceId, FString(TEXT("second")));
	TestTrue(TEXT("Finals never repeat a previous delta"), Results[2].Delta.IsEmpty() && Results[3].Delta.IsEmpty());
	TestEqual(TEXT("Completed records are released"), Gpt->Items.Num(), 0);
	Gpt->Cancel(); Gpt->Transcript(Update);
	TestEqual(TEXT("Cancelled session suppresses late result"), Results.Num(), 4);
	const TArray<uint8> Bytes = UJWNU_GptLiveTranscriptor::EncodePCM({-2.0f, 0.0f, 2.0f}, 0, 3);
	TestEqual(TEXT("PCM16 bytes"), Bytes.Num(), 6);
	TestTrue(TEXT("Clipped signed little-endian PCM"), Bytes[0] == 0 && Bytes[1] == 128 && Bytes[2] == 0 && Bytes[3] == 0 && Bytes[4] == 255 && Bytes[5] == 127);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_GptLiveLifetimeTest,
	"JWNetworkUtility.OpenAI.Transcription.GptLiveTranscriptor.Lifetime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_GptLiveLifetimeTest::RunTest(const FString& Parameters)
{
	UJWNU_GptLiveTranscriptor* Gpt = NewObject<UJWNU_GptLiveTranscriptor>();
	int32 Errors = 0, Finals = 0, Finished = 0;
	Gpt->OnErrorNative.AddLambda([&Errors](const FString&) { ++Errors; });
	Gpt->OnFinishedNative.AddLambda([&Finished]() { ++Finished; });
	AddExpectedError(TEXT("\\[GPT\\.Error\\]"), EAutomationExpectedErrorFlags::Contains, 4);
	Gpt->bActive = Gpt->bReady = true;
	Gpt->AwaitingCommits.Add(10);
	Gpt->Poll(9);
	TestEqual(TEXT("Commit deadline does not expire early"), Errors, 0);
	Gpt->Poll(10); Gpt->Poll(11);
	TestEqual(TEXT("Missing commit produces one error and clears queue"), Errors, 1);
	TestFalse(TEXT("Timeout makes wrapper inactive"), Gpt->IsActive());
	Gpt->bActive = Gpt->bReady = true;
	Gpt->Items.FindOrAdd(TEXT("pending")).Deadline = 20;
	Gpt->Poll(20);
	TestEqual(TEXT("Missing final also expires"), Errors, 2);
	Gpt->bActive = Gpt->bReady = true;
	FJWNU_OpenAITranscript Update; Update.ItemId = TEXT("large"); Update.Delta = FString::ChrN(16385, TCHAR('x'));
	Gpt->Transcript(Update);
	TestEqual(TEXT("Text bound fails instead of truncating a final"), Errors, 3);
	Gpt->bActive = Gpt->bReady = true;
	Gpt->LastCommittedId = TEXT("first"); Gpt->AwaitingCommits.Add(30);
	FJWNU_OpenAITranscriptionCommit Commit; Commit.ItemId = TEXT("second"); Commit.PreviousItemId = TEXT("wrong");
	Gpt->Committed(Commit);
	TestEqual(TEXT("Broken previous item chain fails explicitly"), Errors, 4);
	Gpt->bActive = Gpt->bReady = true;
	Gpt->CommitOrder = {TEXT("one"), TEXT("two")};
	for (const FString& Id : Gpt->CommitOrder) { auto& Item = Gpt->Items.FindOrAdd(Id); Item.Text = Id; Item.bFinal = true; }
	Gpt->OnTranscriptNative.AddLambda([Gpt, &Finals](const FJWNU_GptLiveTranscript&) { ++Finals; Gpt->Cancel(); });
	Gpt->FlushFinals();
	TestEqual(TEXT("Reentrant cancel prevents queued second final"), Finals, 1);
	TestEqual(TEXT("Cancel never emits Finished"), Finished, 0);
	Gpt->bActive = true;
	Gpt->Finish(); Gpt->Finish();
	TestEqual(TEXT("Finish during preparation completes once"), Finished, 1);
	return true;
}

#endif
