// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWNU_ScribeTranscriptor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace JWNU::ScribeTest
{
class FRun : public IAutomationLatentCommand
{
public:
	FRun(FAutomationTestBase* InTest, FString InURL) : Test(InTest), URL(MoveTemp(InURL)) {}
	virtual bool Update() override
	{
		if (!Instance)
		{
			Instance = NewObject<UGameInstance>(GEngine);
			Instance->AddToRoot(); Instance->InitializeStandalone();
			Backend = UJWNU_ScribeTranscriptor::CreateScribeTranscriptor(Instance->GetWorld());
			if (!Test->TestNotNull(TEXT("Create Scribe"), Backend)) { Cleanup(); return true; }
			Backend->AddToRoot();
			Backend->OnTranscriptionReadyNative.AddLambda([this]() { bReady = true; });
			Backend->OnTranscriptionFinishedNative.AddLambda([this]() { bFinished = true; });
			Backend->OnTranscriptionErrorNative.AddLambda([this](const FString& Message) { Error = Message; });
			Backend->OnTranscriptionUpdateNative.AddLambda([this](const FJWNU_TranscriptUpdate& Update) { Results.Add(Update); });
			FJWNU_ScribeOptions Options;
			Options.Endpoint = URL;
			Options.Language = TEXT("ko");
			Options.FinishDrainSeconds = 2;
			Started = FPlatformTime::Seconds();
			if (!Test->TestTrue(TEXT("Start accepts local fixture"), Backend->Start(Options, FString()))) { Cleanup(); return true; }
		}
		// InitializeStandalone 월드는 자동 게임 루프가 없으므로 종료 deadline tick을 직접 진행한다.
		Backend->Tick(0.f);
		if (!Error.IsEmpty() || FPlatformTime::Seconds() - Started > 15)
		{
			Test->AddError(Error.IsEmpty() ? TEXT("Scribe fixture timed out") : Error);
			Cleanup(); return true;
		}
		if (bReady && !bSent)
		{
			bSent = true;
			TArray<float> Audio; Audio.SetNumZeroed(8017);
			Audio[0] = -1.f; Audio[1] = 1.f;
			Backend->AppendAudio(Audio);
			Backend->CommitUtterance(); // 서버 VAD이므로 wire Commit이 발생하면 fixture가 실패한다.
			Backend->Finish(); Backend->Finish();
		}
		if (!bFinished) return false;
		TArray<FJWNU_TranscriptUpdate> Finals;
		for (const auto& Update : Results) if (Update.bFinal) Finals.Add(Update);
		Test->TestEqual(TEXT("Two utterances finalize; timestamp metadata is ignored"), Finals.Num(), 2);
		if (Finals.Num() == 2)
		{
			Test->TestEqual(TEXT("UTF-8 final preserved"), Finals[0].Text, FString(TEXT("대형 유지")));
			Test->TestEqual(TEXT("Repeated utterance text remains valid"), Finals[0].Text, Finals[1].Text);
			Test->TestTrue(TEXT("Repeated utterances have distinct IDs"), Finals[0].SegmentId != Finals[1].SegmentId);
		}
		Test->TestEqual(TEXT("Corrected partial snapshots are not appended"), Results.Num(), 5);
		if (Results.Num() >= 2) Test->TestEqual(TEXT("Second snapshot corrects first"), Results[1].Text, FString(TEXT("대형 유지")));
		for (const auto& Update : Results) Test->TestTrue(TEXT("Scribe supplies no synthetic Delta"), Update.Delta.IsEmpty());
		Cleanup(); return true;
	}
private:
	void Cleanup()
	{
		if (Backend) { Backend->Cancel(); Backend->RemoveFromRoot(); Backend = nullptr; }
		if (Instance) { Instance->Shutdown(); Instance->RemoveFromRoot(); Instance = nullptr; }
	}
	FAutomationTestBase* Test;
	FString URL, Error;
	UGameInstance* Instance = nullptr;
	UJWNU_ScribeTranscriptor* Backend = nullptr;
	TArray<FJWNU_TranscriptUpdate> Results;
	double Started = 0;
	bool bReady = false, bSent = false, bFinished = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_ScribeIntegrationTest, "JWNetworkUtility.Scribe.Integration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_ScribeIntegrationTest::RunTest(const FString& Parameters)
{
	FString URL;
	if (!FParse::Value(FCommandLine::Get(), TEXT("JWNUScribeTestURL="), URL))
	{ AddError(TEXT("Run TestServer/run_scribe_tests.py to provide the loopback fixture.")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(JWNU::ScribeTest::FRun(this, URL));
	return true;
}

#endif
