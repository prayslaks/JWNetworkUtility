// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWNU_ScribeTranscriptor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_ScribeProtocolTest, "JWNetworkUtility.Scribe.Protocol", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_ScribeProtocolTest::RunTest(const FString& Parameters)
{
	auto* Scribe = NewObject<UJWNU_ScribeTranscriptor>();
	TestEqual(TEXT("Provider owns boundaries"), Scribe->GetBoundaryMode(), EJWNU_TranscriptBoundaryMode::ProviderManaged);
	Scribe->bActive = true;
	int32 Ready = 0, Errors = 0, Finished = 0;
	TArray<FJWNU_TranscriptUpdate> Results;
	Scribe->OnTranscriptionReadyNative.AddLambda([&Ready]() { ++Ready; });
	Scribe->OnTranscriptionUpdateNative.AddLambda([&Results](const FJWNU_TranscriptUpdate& Value) { Results.Add(Value); });
	Scribe->OnTranscriptionErrorNative.AddLambda([&Errors](const FString&) { ++Errors; });
	Scribe->OnTranscriptionFinishedNative.AddLambda([&Finished]() { ++Finished; });
	Scribe->Receive(TEXT("{\"message_type\":\"session_started\",\"config\":{\"audio_format\":\"pcm_16000\",\"sample_rate\":16000,\"model_id\":\"scribe_v2_realtime\"}}"));
	TestEqual(TEXT("Only session readiness enables input"), Ready, 1);
	Scribe->Receive(TEXT("{\"message_type\":\"partial_transcript\",\"text\":\"대열\"}"));
	Scribe->Receive(TEXT("{\"message_type\":\"partial_transcript\",\"text\":\"대형 유지\"}"));
	Scribe->Receive(TEXT("{\"message_type\":\"committed_transcript\",\"text\":\"대형 유지\"}"));
	Scribe->Receive(TEXT("{\"message_type\":\"committed_transcript_with_timestamps\",\"text\":\"대형 유지\"}"));
	Scribe->Receive(TEXT("{\"message_type\":\"committed_transcript\",\"text\":\"\"}"));
	Scribe->Receive(TEXT("{\"message_type\":\"committed_transcript\",\"text\":\"대형 유지\"}"));
	TestEqual(TEXT("Metadata never duplicates a Final; repeated utterances remain"), Results.Num(), 4);
	if (Results.Num() == 4)
	{
		TestEqual(TEXT("Partial corrections replace the entire text"), Results[1].Text, FString(TEXT("대형 유지")));
		TestTrue(TEXT("Snapshot provider does not invent Delta"), Results[1].Delta.IsEmpty());
		TestEqual(TEXT("Partial and Final share an identity"), Results[0].SegmentId, Results[2].SegmentId);
		TestNotEqual(TEXT("Repeated text in next segment has a new identity"), Results[2].SegmentId, Results[3].SegmentId);
	}
	Scribe->CommitUtterance();
	TestFalse(TEXT("External commit cannot finish server VAD"), Scribe->bFinishing);
	Scribe->Cancel();
	Scribe->Receive(TEXT("{\"message_type\":\"partial_transcript\",\"text\":\"stale\"}"));
	TestEqual(TEXT("Cancel suppresses stale results"), Results.Num(), 4);
	TestEqual(TEXT("Cancel does not finish"), Finished, 0);
	Scribe->bActive = Scribe->bReady = true;
	Scribe->OnTranscriptionUpdateNative.AddLambda([Scribe](const FJWNU_TranscriptUpdate&) { Scribe->Cancel(); });
	Scribe->Receive(TEXT("{\"message_type\":\"committed_transcript\",\"text\":\"cancel\"}"));
	TestFalse(TEXT("Result callback can cancel safely"), Scribe->bActive);
	Scribe->bActive = true;
	Scribe->Receive(TEXT("invalid JSON"));
	TestEqual(TEXT("Malformed protocol fails once"), Errors, 1);
	TestFalse(TEXT("Failure cleans up active state"), Scribe->bActive);
	return true;
}

#endif
