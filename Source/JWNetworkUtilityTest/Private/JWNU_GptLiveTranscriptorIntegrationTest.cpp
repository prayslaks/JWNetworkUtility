// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#if WITH_DEV_AUTOMATION_TESTS

#include "JWNU_GptLiveTranscriptor.h"
#include "JWNU_OpenAITranscriptionTestReceiver.h"
#include "UObject/GarbageCollection.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace JWNU::GptTranscriptorTest
{
class FRun : public IAutomationLatentCommand
{
public:
	explicit FRun(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		if (!Instance)
		{
			Instance = NewObject<UGameInstance>(GEngine);
			Instance->AddToRoot(); Instance->InitializeStandalone();
			Backend = UJWNU_GptLiveTranscriptor::CreateGptLiveTranscriptor(Instance->GetWorld());
			if (!Test->TestNotNull(TEXT("Creates standalone plugin transcriptor"), Backend)) { Cleanup(); return true; }
			Receiver = NewObject<UJWNU_GptLiveTranscriptorTestReceiver>(); Receiver->AddToRoot();
			Backend->OnReady.AddDynamic(Receiver, &UJWNU_GptLiveTranscriptorTestReceiver::Ready);
			Backend->OnFinished.AddDynamic(Receiver, &UJWNU_GptLiveTranscriptorTestReceiver::Finished);
			Backend->OnTranscript.AddDynamic(Receiver, &UJWNU_GptLiveTranscriptorTestReceiver::Transcript);
			Backend->OnError.AddDynamic(Receiver, &UJWNU_GptLiveTranscriptorTestReceiver::Error);
			FString Base = TEXT("ws://127.0.0.1:18573");
			FParse::Value(FCommandLine::Get(), TEXT("JWNUTranscriptionTestURL="), Base);
			Options.Endpoint = Base + TEXT("/realtime?intent=transcription&mode=reverse");
			Backend->OnReadyNative.AddLambda([this]() { bReady = true; });
			Backend->OnFinishedNative.AddLambda([this]() { bFinished = true; });
			Backend->OnErrorNative.AddLambda([this](const FString& Message) { Error = Message; });
			Backend->OnTranscriptNative.AddLambda([this](const FJWNU_GptLiveTranscript& Value) { Results.Add(Value);
				if (Runs == 1 && Value.bFinal) { Backend->Cancel(); bCancelled = true; } });
			Started = FPlatformTime::Seconds();
			Test->TestTrue(TEXT("Start accepted"), Backend->Start(Options, FString()));
			TWeakObjectPtr<UJWNU_GptLiveTranscriptor> Weak = Backend;
			CollectGarbage(RF_NoFlags);
			if (!Test->TestTrue(TEXT("Subsystem retains active wrapper through GC"), Weak.IsValid())) { Backend = nullptr; Cleanup(); return true; }
		}
		if (!Error.IsEmpty() || FPlatformTime::Seconds() - Started > 15)
		{
			Test->AddError(Error.IsEmpty() ? TEXT("GPT adapter integration timed out") : Error);
			Cleanup(); return true;
		}
		if (Stage == 0 && bReady)
		{
			TArray<float> Audio; Audio.SetNumZeroed(5000);
			Backend->AppendAudio(Audio); // 100ms보다 큰 입력을 여러 전송으로 분리한다.
			Stage = 1;
		}
		if (Stage == 1 && !Results.IsEmpty())
		{
			Test->TestFalse(TEXT("Real socket yields partial before commit"), Results[0].bFinal);
			Backend->CommitUtterance();
			TArray<float> Tail; Tail.SetNumZeroed(480);
			Backend->AppendAudio(Tail);
			Backend->Finish(); // 20ms 발화가 100ms로 패딩되어 서버에서 접수되어야 한다.
			Stage = 2;
		}
		if (bCancelled)
		{
			Test->TestFalse(TEXT("Native final callback can cancel"), Backend->IsActive());
			Test->TestEqual(TEXT("Cancel does not finish"), Receiver->FinishedCount, 1);
			Test->TestEqual(TEXT("Cancel does not report failure"), Receiver->ErrorCount, 0);
			for (const auto& Value : Receiver->Transcripts) Test->TestFalse(TEXT("Cancel suppresses BP final from old session"), Value.bFinal);
			Cleanup(); return true;
		}
		if (!bFinished) return false;
		TArray<FJWNU_GptLiveTranscript> Finals;
		for (const FJWNU_GptLiveTranscript& Value : Results) if (Value.bFinal) Finals.Add(Value);
		Test->TestEqual(TEXT("Both socket turns finalize including padded tail"), Finals.Num(), 2);
		if (Finals.Num() == 2)
		{
			Test->TestEqual(TEXT("Reversed server finals are reordered"), Finals[0].UtteranceId, FString(TEXT("item_1")));
			Test->TestEqual(TEXT("Second turn identity preserved"), Finals[1].UtteranceId, FString(TEXT("item_2")));
			Test->TestTrue(TEXT("UTF-8 final replaces partial"), Finals[0].Text.Contains(TEXT("최종")));
		}
		Test->TestEqual(TEXT("Dynamic Ready delivered"), Receiver->ReadyCount, 1);
		Test->TestEqual(TEXT("Dynamic Finished delivered"), Receiver->FinishedCount, 1);
		Test->TestEqual(TEXT("Dynamic snapshots match native events"), Receiver->Transcripts.Num(), Results.Num());
		Test->TestFalse(TEXT("Finish leaves inactive reusable wrapper"), Backend->IsActive());
		++Runs; Stage = 0; bReady = bFinished = false; Results.Reset(); Receiver->Transcripts.Reset();
		Test->TestTrue(TEXT("Same wrapper restarts with a fresh socket"), Backend->Start(Options, FString()));
		Started = FPlatformTime::Seconds();
		return false;
	}
private:
	void Cleanup()
	{
		if (Backend) { Backend->Cancel(); Backend = nullptr; }
		if (Receiver) { Receiver->RemoveFromRoot(); Receiver = nullptr; }
		UWorld* World = Instance->GetWorld();
		Instance->Shutdown(); World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		Instance->RemoveFromRoot(); Instance = nullptr;
	}
	FAutomationTestBase* Test;
	UGameInstance* Instance = nullptr;
	UJWNU_GptLiveTranscriptor* Backend = nullptr;
	TArray<FJWNU_GptLiveTranscript> Results;
	FString Error;
	double Started = 0;
	int32 Stage = 0, Runs = 0;
	bool bCancelled = false;
	FJWNU_OpenAITranscriptionOptions Options;
	UJWNU_GptLiveTranscriptorTestReceiver* Receiver = nullptr;
	bool bReady = false;
	bool bFinished = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_GptLiveTranscriptorIntegrationTest,
	"JWNetworkUtility.OpenAI.Transcription.GptLiveTranscriptor.FastAPI", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJWNU_GptLiveTranscriptorIntegrationTest::RunTest(const FString& Parameters)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUTranscriptionIntegration")))
	{
		AddInfo(TEXT("Requires -JWNUTranscriptionIntegration and local TestServer/run_transcription_tests.py.")); return true;
	}
	ADD_LATENT_AUTOMATION_COMMAND(JWNU::GptTranscriptorTest::FRun(this));
	return true;
}

#endif
