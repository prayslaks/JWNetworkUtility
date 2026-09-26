// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_WebSocketTestReceiver.h"
#include "JWNU_BFL_WebSocketClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/GarbageCollection.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"

namespace JWNU::WebSocketTest
{
inline UBlueprint* BuildReceiver(FAutomationTestBase* Test)
{
	auto* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_WebSocketTestReceiver::StaticClass(), GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_WebSocketAutomation")),
		BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("ReceiveGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
	auto* Event = NewObject<UK2Node_Event>(Graph);
	Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UJWNU_WebSocketTestReceiver, ReceiveText), UJWNU_WebSocketTestReceiver::StaticClass());
	Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
	auto* Record = NewObject<UK2Node_CallFunction>(Graph);
	Record->SetFromFunction(UJWNU_WebSocketTestReceiver::StaticClass()->FindFunctionByName(TEXT("RecordText")));
	Graph->AddNode(Record); Record->CreateNewGuid(); Record->AllocateDefaultPins();
	const auto* Schema = GetDefault<UEdGraphSchema_K2>();
	Test->TestTrue(TEXT("BP text wire"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Message")), Record->FindPinChecked(TEXT("Message"))));
	Test->TestTrue(TEXT("BP execution wire"), Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Record->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
	FKismetEditorUtilities::CompileBlueprint(BP);
	Test->TestTrue(TEXT("BP compiled"), BP->Status != BS_Error);
	return BP;
}

class FIntegration : public IAutomationLatentCommand
{
public:
	explicit FIntegration(FAutomationTestBase* InTest) : Test(InTest) {}
	virtual bool Update() override
	{
		if (!Instance)
		{
			BaseURL = TEXT("ws://127.0.0.1:18573");
			FParse::Value(FCommandLine::Get(), TEXT("JWNUWebSocketTestURL="), BaseURL);
			Instance = NewObject<UGameInstance>(GEngine); Instance->AddToRoot(); Instance->InitializeStandalone();
			World = Instance->GetWorld();
			Blueprint = BuildReceiver(Test); Blueprint->AddToRoot();
			for (int32 Index = 0; Index < 300000; ++Index) { Payload.Add(static_cast<uint8>(Index % 256)); }
			for (int32 Index = 0; Index < 20000; ++Index) { LargeText += TEXT("안녕 ✈🙂"); }
			if (FParse::Param(FCommandLine::Get(), TEXT("JWNUWebSocketExpectTlsFailure"))) { Stage = 18; }
		}
		if (Stage == 17 || Stage == 19) { Cleanup(); return true; }
		if (!Receiver) { Start(); }
		const double Now = FPlatformTime::Seconds();
		if (Now - Started > 8)
		{
			Test->AddError(FString::Printf(TEXT("WebSocket stage %d timed out (state %d)"), Stage, static_cast<int32>(Connection->GetState())));
			Cleanup(); return true;
		}
		if (Stage == 0 && Receiver->Texts.Num() >= 1 && Receiver->BinaryCount >= 1 && !bExtraSent)
		{
			Test->TestEqual(TEXT("UTF-8 JSON echo via BP"), Receiver->Texts[0], Echo);
			Test->TestTrue(TEXT("300KB binary echo"), Receiver->LastBinary == Payload);
			CollectGarbage(RF_NoFlags);
			Test->TestTrue(TEXT("GC preserves active connection"), Connection->IsConnected());
			Test->TestTrue(TEXT("Large UTF-8 text accepted"), Connection->SendText(LargeText));
			Test->TestTrue(TEXT("Empty text accepted"), Connection->SendText(TEXT("")));
			Test->TestTrue(TEXT("Empty binary accepted"), Connection->SendBinary({}));
			bExtraSent = true; IdleSince = Now;
		}
		if (Stage == 0 && bExtraSent && Now - IdleSince > .4 && Connection->IsConnected())
		{
			Test->TestEqual(TEXT("Text messages include large and empty"), Receiver->Texts.Num(), 3);
			if (Receiver->Texts.Num() >= 2) { Test->TestTrue(TEXT("Large fragmented UTF-8 text preserved"), Receiver->Texts[1] == LargeText); }
			Test->TestEqual(TEXT("Empty text preserved"), Receiver->Texts.Last(), FString());
			Test->TestEqual(TEXT("Binary messages include empty"), Receiver->BinaryCount, 2);
			Test->TestEqual(TEXT("Empty binary preserved"), Receiver->EmptyBinaryCount, 1);
			Connection->Close(); Test->TestFalse(TEXT("Double close rejected"), Connection->Close());
		}
		if (Stage == 1 && Receiver->Texts.Num() >= 4 && Connection->IsConnected()) { Connection->Close(); }
		if (Stage == 10 && Receiver->Texts.Num() >= NativeConnected && Connection->IsConnected()) { Connection->Close(); }
		if (Stage == 10 && Receiver->ClosedCount == 1 && !bReconnected)
		{
			bReconnected = true;
			Test->TestTrue(TEXT("Explicit reconnect same handle"), Connection->Connect(BaseURL + TEXT("/ws/echo"), {}));
		}
		if (Stage == 11 && !Receiver->Texts.IsEmpty() && Connection->IsConnected()) { FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false); }
		if (Stage == 14 && Now - Started > .1 && Connection->GetState() == EJWNU_WebSocketState::Connecting) { Connection->Close(); }
		if (Stage == 16 && Connection->IsConnected())
		{
			Instance->Shutdown(); bShutdown = true;
		}
		const int32 ExpectedClosed = Stage == 10 ? 2 : 1;
		if (Receiver->ClosedCount < ExpectedClosed) { return false; }
		if (!Finished) { Finished = Now; return false; }
		if (Now - Finished < .15) { return false; }
		Validate();
		Receiver->RemoveFromRoot(); Receiver = nullptr; Connection = nullptr; ++Stage;
		return false;
	}
private:
	void Start()
	{
		Started = FPlatformTime::Seconds(); Finished = 0; IdleSince = 0; bExtraSent = false; bReconnected = false;
		NativeConnected = 0; NativeClosed = 0; NativeErrors = 0; NativeTexts = 0; NativeBinary = 0;
		Test->AddInfo(FString::Printf(TEXT("WebSocket stage %d"), Stage));
		Receiver = NewObject<UJWNU_WebSocketTestReceiver>(GetTransientPackage(), Blueprint->GeneratedClass.Get());
		Receiver->AddToRoot();
		FJWNU_WebSocketOptions Options;
		FString Path = TEXT("/ws/echo");
		if (Stage == 1) { Path += TEXT("?mode=headers&push_count=3&delay=0.05"); }
		if (Stage == 2) { Path += TEXT("?mode=server_close"); }
		if (Stage == 3) { Path += TEXT("?mode=drop"); }
		if (Stage == 5) { Path += TEXT("?mode=reject"); }
		if (Stage == 6 || Stage == 14) { Path += TEXT("?mode=before_open"); Options.ConnectTimeoutSeconds = Stage == 6 ? .1f : 3.f; }
		if (Stage == 7) { Path += TEXT("?mode=large_binary"); Options.MaxMessageBytes = 128; }
		if (Stage == 8) { Options.MaxQueuedBytes = 1; }
		if (Stage == 12) { Options.MaxMessageBytes = 32; Options.MaxSendQueuedBytes = 1; }
		if (Stage == 13) { Options.Headers.Add(TEXT("X-Test"), TEXT("invalid\r\nheader")); }
		if (Stage == 0 || Stage == 1) { Options.Headers.Add(TEXT("X-JWNU-Test"), TEXT("echo-header")); Options.Protocols.Add(TEXT("jwnu.echo")); }
		// 편의 노드도 Handle을 반환한 뒤 첫 Tick에 연결하므로 아래 바인딩이 먼저 실행된다.
		Connection = UJWNU_BFL_WebSocketClient::ConnectWebSocket(World, Stage == 4 ? TEXT("http://bad-url") : BaseURL + Path, Options);
		Test->TestNotNull(TEXT("BP connection handle"), Connection);
		if (!Connection) { return; }
		Connection->OnConnected.AddDynamic(Receiver, &UJWNU_WebSocketTestReceiver::ReceiveConnected);
		Connection->OnTextMessage.AddDynamic(Receiver, &UJWNU_WebSocketTestReceiver::ReceiveText);
		Connection->OnBinaryMessage.AddDynamic(Receiver, &UJWNU_WebSocketTestReceiver::ReceiveBinary);
		Connection->OnClosed.AddDynamic(Receiver, &UJWNU_WebSocketTestReceiver::ReceiveClosed);
		Connection->OnError.AddDynamic(Receiver, &UJWNU_WebSocketTestReceiver::ReceiveError);
		Connection->OnTextMessageNative.AddLambda([this](const FString&) { ++NativeTexts; });
		Connection->OnBinaryMessageNative.AddLambda([this](const TArray<uint8>&) { ++NativeBinary; });
		Connection->OnClosedNative.AddLambda([this](const FJWNU_WebSocketCloseInfo&) { ++NativeClosed; });
		Connection->OnErrorNative.AddLambda([this](const FJWNU_WebSocketError&) { ++NativeErrors; });
		Connection->OnConnectedNative.AddLambda([this]()
		{
			++NativeConnected;
			if (Stage == 18) { Test->AddError(TEXT("Untrusted TLS certificate was accepted")); Connection->Close(); }
			Test->TestFalse(TEXT("Concurrent Connect rejected"), Connection->Connect(BaseURL + TEXT("/ws/echo"), {}));
			if (Stage == 0)
			{
				Test->TestTrue(TEXT("Send text"), Connection->SendText(Echo));
				Test->TestTrue(TEXT("Send binary"), Connection->SendBinary(Payload));
			}
			if (Stage == 10 || Stage == 11) { Connection->SendText(TEXT("roundtrip")); }
			if (Stage == 15)
			{
				FWorldDelegates::OnWorldCleanup.Broadcast(World, false, false);
				CollectGarbage(RF_NoFlags);
			}
			if (Stage == 12)
			{
				Test->TestFalse(TEXT("Oversized send rejected"), Connection->SendBinary(Payload));
				Test->TestFalse(TEXT("Oversized text rejected"), Connection->SendText(FString::ChrN(100, 'x')));
				Test->TestFalse(TEXT("Full send queue rejected"), Connection->SendText(TEXT("ok")));
				Test->TestFalse(TEXT("Reserved close code rejected"), Connection->Close(1006));
				Test->TestFalse(TEXT("Long close reason rejected"), Connection->Close(1000, FString::ChrN(124, 'x')));
				Test->TestTrue(TEXT("Still connected after rejected operations"), Connection->IsConnected());
				Connection->Close();
			}
		});
		Test->TestFalse(TEXT("Send before connected rejected"), Connection->SendText(TEXT("early")));
		if (Stage == 9) { Connection->Close(); }
	}
	void Validate()
	{
		const bool bFailure = Stage == 3 || Stage == 4 || Stage == 5 || Stage == 6 || Stage == 7 || Stage == 8 || Stage == 13 || Stage == 18;
		if (!bFailure && Receiver->ErrorCount) { Test->AddError(FString::Printf(TEXT("Stage %d unexpected error: %s"), Stage, *Receiver->LastError.Message)); }
		Test->TestEqual(TEXT("Closed exactly once per attempt"), Receiver->ClosedCount, Stage == 10 ? 2 : 1);
		Test->TestEqual(TEXT("Failure notification count"), Receiver->ErrorCount, bFailure ? 1 : 0);
		Test->TestFalse(TEXT("Terminal connection inactive"), Connection->IsActive());
		Test->TestFalse(TEXT("No send after close"), Connection->SendBinary({1}));
		if (Stage == 15) { Test->TestEqual(TEXT("No Connected after reentrant Closed"), Receiver->ConnectedCount, 0); }
		else { Test->TestEqual(TEXT("Native and BP connected"), NativeConnected, Receiver->ConnectedCount); }
		Test->TestEqual(TEXT("Native and BP closed"), NativeClosed, Receiver->ClosedCount);
		Test->TestEqual(TEXT("Native and BP errors"), NativeErrors, Receiver->ErrorCount);
		Test->TestEqual(TEXT("Native and actual BP text graph"), NativeTexts, Receiver->Texts.Num());
		Test->TestEqual(TEXT("Native and BP binary"), NativeBinary, Receiver->BinaryCount);
		if (Stage == 1 && Test->TestTrue(TEXT("Header response received"), !Receiver->Texts.IsEmpty()))
		{
			Test->TestTrue(TEXT("Upgrade header delivered"), Receiver->Texts[0].Contains(TEXT("echo-header")));
			Test->TestTrue(TEXT("Subprotocol negotiated"), Receiver->Texts[0].Contains(TEXT("jwnu.echo")));
		}
		if (Stage == 2)
		{
			Test->TestEqual(TEXT("Server close code retained"), Receiver->LastClose.Code, 4001);
			Test->TestFalse(TEXT("Server initiated"), Receiver->LastClose.bWasLocal);
		}
		if (Stage == 0 || Stage == 2) { Test->TestTrue(TEXT("Close handshake completed"), Receiver->LastClose.bWasClean); }
		if (Stage == 4 || Stage == 13) { Test->TestEqual(TEXT("Invalid configuration"), Receiver->LastError.Code, EJWNU_WebSocketError::InvalidConfiguration); }
		if (Stage == 6) { Test->TestEqual(TEXT("Connection timeout"), Receiver->LastError.Code, EJWNU_WebSocketError::Timeout); }
		if (Stage == 18)
		{
			Test->TestEqual(TEXT("Untrusted TLS rejected during connection"), Receiver->LastError.Code, EJWNU_WebSocketError::Connection);
			Test->TestEqual(TEXT("No Connected on rejected TLS"), Receiver->ConnectedCount, 0);
		}
		if (Stage == 7 || Stage == 8) { Test->TestEqual(TEXT("Receive bounds"), Receiver->LastError.Code, EJWNU_WebSocketError::BufferLimit); }
		if (Stage == 9 || Stage == 14) { Test->TestEqual(TEXT("Cancelled before connected"), Receiver->ConnectedCount, 0); }
		if (Stage == 10) { Test->TestEqual(TEXT("Both sessions delivered"), Receiver->Texts.Num(), 2); }
		if (Stage == 11 || Stage == 15 || Stage == 16)
		{
			Test->TestTrue(TEXT("World shutdown local"), Receiver->LastClose.bWasLocal);
			Test->TestFalse(TEXT("Ended world cannot reconnect"), Connection->Connect(BaseURL + TEXT("/ws/echo"), {}));
		}
	}
	void Cleanup()
	{
		if (!bShutdown) { Instance->Shutdown(); }
		if (Receiver) { Receiver->RemoveFromRoot(); Receiver = nullptr; }
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
		Instance->RemoveFromRoot(); Blueprint->RemoveFromRoot();
	}
	FAutomationTestBase* Test;
	UGameInstance* Instance = nullptr;
	UWorld* World = nullptr;
	UBlueprint* Blueprint = nullptr;
	UJWNU_WebSocketConnection* Connection = nullptr;
	UJWNU_WebSocketTestReceiver* Receiver = nullptr;
	FString BaseURL;
	FString LargeText;
	const FString Echo = TEXT("{\"Text\":\"안녕 ✈\",\"Index\":1}");
	TArray<uint8> Payload;
	int32 Stage = 0, NativeConnected = 0, NativeClosed = 0, NativeErrors = 0, NativeTexts = 0, NativeBinary = 0;
	double Started = 0, Finished = 0, IdleSince = 0;
	bool bExtraSent = false, bReconnected = false, bShutdown = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_WebSocketIntegrationTest, "JWNetworkUtility.WebSocket.FastAPI",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_WebSocketIntegrationTest::RunTest(const FString& Parameters)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("JWNUWebSocketIntegration")))
	{
		AddInfo(TEXT("Skipped; run TestServer/run_websocket_tests.py to start the fixture."));
		return true;
	}
	ADD_LATENT_AUTOMATION_COMMAND(JWNU::WebSocketTest::FIntegration(this));
	return true;
}
#endif
