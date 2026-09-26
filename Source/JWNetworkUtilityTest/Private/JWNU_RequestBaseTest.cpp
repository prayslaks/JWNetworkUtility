// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_RequestTestReceiver.h"
#include "JWNU_HttpRequest.h"
#include "JWNU_ApiRequest.h"
#include "JWNU_SseRequest.h"
#include "JWNU_TypeSafeRequest.h"
#include "JWNU_GIS_ApiHostProvider.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_RequestBaseTest, "JWNetworkUtility.API.CommonRequest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_RequestBaseTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UBlueprint> BP(FKismetEditorUtilities::CreateBlueprint(UJWNU_RequestTestReceiver::StaticClass(), GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_CommonRequest")), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass()));
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP.Get(), TEXT("CommonGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(BP.Get(), Graph);
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    for (bool bFinish : {false, true})
    {
        auto* Event = NewObject<UK2Node_Event>(Graph);
        Event->EventReference.SetExternalMember(bFinish ? TEXT("Finished") : TEXT("CancelInBlueprint"), UJWNU_RequestTestReceiver::StaticClass());
        Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
        auto* Call = NewObject<UK2Node_CallFunction>(Graph);
        Call->SetFromFunction((bFinish ? UJWNU_RequestTestReceiver::StaticClass() : UJWNU_RequestBase::StaticClass())->FindFunctionByName(bFinish ? TEXT("Record") : TEXT("Cancel")));
        Graph->AddNode(Call); Call->CreateNewGuid(); Call->AllocateDefaultPins();
        TestTrue(TEXT("Common BP execution wire"), Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Call->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        TestTrue(TEXT("Base request pin"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Request")), Call->FindPinChecked(bFinish ? TEXT("Request") : UEdGraphSchema_K2::PN_Self)));
        if (bFinish) { TestTrue(TEXT("Common state pin"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("State")), Call->FindPinChecked(TEXT("State")))); }
    }
    FKismetEditorUtilities::CompileBlueprint(BP.Get());
    TestTrue(TEXT("Common BP compiles"), BP->Status != BS_Error);
    TStrongObjectPtr<UJWNU_RequestTestReceiver> Receiver(NewObject<UJWNU_RequestTestReceiver>(GetTransientPackage(), BP->GeneratedClass.Get()));
    TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>(GEngine));
    Instance->InitializeStandalone(); auto* World = Instance->GetWorld();
    auto* Hosts = Instance->GetSubsystem<UJWNU_GIS_ApiHostProvider>();
    FindFProperty<FMapProperty>(Hosts->GetClass(), TEXT("ServiceTypeToHostMap"))->ContainerPtrToValuePtr<TMap<EJWNU_ServiceType, FString>>(Hosts)->Add(EJWNU_ServiceType::GameServer, TEXT("http://127.0.0.1:18573"));
    TArray<TStrongObjectPtr<UJWNU_RequestBase>> Requests;
    Requests.Emplace(UJWNU_HttpRequest::CreateHttpRequest(World)); Requests.Emplace(UJWNU_ApiRequest::CreateApiRequest(World));
    Requests.Emplace(UJWNU_SseRequest::CreateSseRequest(World)); Requests.Emplace(UJWNU_SseApiRequest::CreateSseApiRequest(World));
    Requests.Emplace(UJWNU_TypeSafeRequest::CreateTypeSafeRequest(World));
    int32 ExpectedCount = 0;
    for (const auto& Request : Requests)
    {
        TestEqual(TEXT("Created common state"), Request->GetState(), EJWNU_RequestState::Created);
        Request->Cancel(); TestEqual(TEXT("Cancel before Start is a no-op"), Request->GetState(), EJWNU_RequestState::Created);
        Request->OnFinished.AddDynamic(Receiver.Get(), &UJWNU_RequestTestReceiver::Finished);
        if (auto* Http = Cast<UJWNU_HttpRequest>(Request.Get())) { Http->Start(EJWNU_HttpMethod::Get, TEXT("http://127.0.0.1:18573/health"), TEXT(""), {}); }
        else if (auto* Api = Cast<UJWNU_ApiRequest>(Request.Get())) { Api->Start(EJWNU_ServiceType::GameServer, EJWNU_HttpMethod::Get, TEXT("/health"), TEXT(""), {}, false); }
        else if (auto* Sse = Cast<UJWNU_SseRequest>(Request.Get())) { Sse->Start(EJWNU_HttpMethod::Get, TEXT("http://127.0.0.1:18573/sse/events"), TEXT(""), {}, {}); }
        else if (auto* SseApi = Cast<UJWNU_SseApiRequest>(Request.Get())) { SseApi->Start(EJWNU_ServiceType::GameServer, EJWNU_HttpMethod::Get, TEXT("/sse/events"), TEXT(""), {}, {}, false); }
        else { CastChecked<UJWNU_TypeSafeRequest>(Request.Get())->Start({}, {}, {}, TEXT("")); }
        TestEqual(TEXT("Common active state"), Request->GetState(), EJWNU_RequestState::Active);
        Receiver->CancelInBlueprint(Request.Get()); Receiver->CancelInBlueprint(Request.Get());
        TestEqual(TEXT("Inherited BP Cancel dispatch"), Request->GetState(), EJWNU_RequestState::Cancelled);
        TestEqual(TEXT("One common BP terminal event"), Receiver->Count, ++ExpectedCount);
        TestEqual(TEXT("BP receives base request"), Receiver->LastRequest.Get(), Request.Get());
        TestEqual(TEXT("BP receives cancelled state"), Receiver->LastState, EJWNU_RequestState::Cancelled);
    }
    Instance->Shutdown(); GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
    return true;
}
#endif
