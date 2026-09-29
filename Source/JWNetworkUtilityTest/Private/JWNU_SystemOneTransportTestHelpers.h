// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "JWNU_SystemOneTransportTestReceiver.h"
#include "JWNU_SystemOneRequest.h"
#include "JWNU_BFL_SystemOne.h"
#include "Misc/AutomationTest.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"

namespace JWNU::SystemOneTransportTest
{
inline UBlueprint* BuildReceiver(FAutomationTestBase* Test)
{
    for (const TCHAR* Name : {TEXT("Evaluate"), TEXT("EvaluateFromEnvironment")})
    {
        const auto* Function = UJWNU_BFL_SystemOne::StaticClass()->FindFunctionByName(Name);
        Test->TestNull(TEXT("Removed evaluation no longer resolves"), Function);
    }
    for (const TCHAR* Name : {TEXT("CreateSystemOneRequest"), TEXT("Start"), TEXT("StartFromEnvironment")})
    {
        Test->TestTrue(TEXT("Explicit request lifecycle is available in BP"), UEdGraphSchema_K2::CanUserKismetCallFunction(UJWNU_SystemOneRequest::StaticClass()->FindFunctionByName(Name)));
    }
    auto* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_SystemOneTransportTestReceiver::StaticClass(), GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_SystemOneAutomation")),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("ReceiveGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
    auto* Event = NewObject<UK2Node_Event>(Graph);
    Event->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UJWNU_SystemOneTransportTestReceiver, Completed), UJWNU_SystemOneTransportTestReceiver::StaticClass());
    Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
    auto* Record = NewObject<UK2Node_CallFunction>(Graph);
    Record->SetFromFunction(UJWNU_SystemOneTransportTestReceiver::StaticClass()->FindFunctionByName(TEXT("Record")));
    Graph->AddNode(Record); Record->CreateNewGuid(); Record->AllocateDefaultPins();
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    auto AddCall = [Graph](UFunction* Function)
    {
        auto* Node = NewObject<UK2Node_CallFunction>(Graph);
        Node->SetFromFunction(Function); Graph->AddNode(Node); Node->CreateNewGuid(); Node->AllocateDefaultPins(); return Node;
    };
    // 실제 Start 호출 노드를 연결해 Options 미연결 상태에서 BP 컴파일과 기본값 생성을 검증한다.
    for (bool bEnvironment : {false, true})
    {
        auto* StartEvent = NewObject<UK2Node_Event>(Graph);
        StartEvent->EventReference.SetExternalMember(bEnvironment ? TEXT("StartEnvironmentWithDefaultOptions") : TEXT("StartWithDefaultOptions"), UJWNU_SystemOneTransportTestReceiver::StaticClass());
        StartEvent->bOverrideFunction = true; Graph->AddNode(StartEvent); StartEvent->CreateNewGuid(); StartEvent->AllocateDefaultPins();
        auto* StartCall = NewObject<UK2Node_CallFunction>(Graph);
        StartCall->SetFromFunction(UJWNU_SystemOneRequest::StaticClass()->FindFunctionByName(bEnvironment ? TEXT("StartFromEnvironment") : TEXT("Start")));
        Graph->AddNode(StartCall); StartCall->CreateNewGuid(); StartCall->AllocateDefaultPins();
        Test->TestTrue(TEXT("Default Options call exec"), Schema->TryCreateConnection(StartEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        Test->TestTrue(TEXT("Default Options call target"), Schema->TryCreateConnection(StartEvent->FindPinChecked(TEXT("Request")), StartCall->FindPinChecked(UEdGraphSchema_K2::PN_Self)));
        for (const TCHAR* Name : {TEXT("State"), TEXT("Questions")})
        { Test->TestTrue(TEXT("Required input connected"), Schema->TryCreateConnection(StartEvent->FindPinChecked(Name), StartCall->FindPinChecked(Name))); }
        Test->TestTrue(TEXT("Options intentionally unconnected"), StartCall->FindPinChecked(TEXT("Options"))->LinkedTo.IsEmpty());
        Test->TestFalse(TEXT("Options uses a default value"), StartCall->FindPinChecked(TEXT("Options"))->bDefaultValueIsIgnored);
    }
    for (bool bEnvironment : {false, true})
    {
        auto* CallEvent = NewObject<UK2Node_Event>(Graph);
        CallEvent->EventReference.SetExternalMember(bEnvironment ? TEXT("CallEnvironmentWithDefaultOptions") : TEXT("CallWithDefaultOptions"), UJWNU_SystemOneTransportTestReceiver::StaticClass());
        CallEvent->bOverrideFunction = true; Graph->AddNode(CallEvent); CallEvent->CreateNewGuid(); CallEvent->AllocateDefaultPins();
        auto* Call = AddCall(UJWNU_BFL_SystemOne::StaticClass()->FindFunctionByName(bEnvironment ? TEXT("CallSystemOneApiFromEnvironment") : TEXT("CallSystemOneApi")));
        auto* Save = AddCall(UJWNU_SystemOneTransportTestReceiver::StaticClass()->FindFunctionByName(TEXT("RecordStartedRequest")));
        Test->TestTrue(TEXT("Immediate BP exec"), Schema->TryCreateConnection(CallEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), Call->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        Test->TestTrue(TEXT("Immediate world context"), Schema->TryCreateConnection(CallEvent->FindPinChecked(TEXT("Context")), Call->FindPinChecked(TEXT("WorldContextObject"))));
        for (const TCHAR* Name : {TEXT("State"), TEXT("Questions")})
        { Test->TestTrue(TEXT("Immediate inputs"), Schema->TryCreateConnection(CallEvent->FindPinChecked(Name), Call->FindPinChecked(Name))); }
        for (const TCHAR* Name : {TEXT("Options"), TEXT("OnCompleted"), TEXT("OnFailed")})
        { Test->TestTrue(TEXT("Immediate optional pin unconnected"), Call->FindPinChecked(Name)->LinkedTo.IsEmpty()); }
        Test->TestTrue(TEXT("Immediate save exec"), Schema->TryCreateConnection(Call->FindPinChecked(UEdGraphSchema_K2::PN_Then), Save->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        Test->TestTrue(TEXT("Immediate return saved"), Schema->TryCreateConnection(Call->GetReturnValuePin(), Save->FindPinChecked(TEXT("Request"))));
    }
    Test->TestTrue(TEXT("BP typed result wire"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Result")), Record->FindPinChecked(TEXT("Result"))));
    Test->TestTrue(TEXT("BP exec wire"), Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Record->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
    auto MakeCall = [Graph](const TCHAR* Name)
    {
        auto* Node = NewObject<UK2Node_CallFunction>(Graph);
        Node->SetFromFunction(UJWNU_BFL_SystemOne::StaticClass()->FindFunctionByName(Name));
        Graph->AddNode(Node); Node->CreateNewGuid(); Node->AllocateDefaultPins(); return Node;
    };
    auto* WithCriteria = MakeCall(TEXT("MakeNoulQuestion"));
    auto* WithoutCriteria = MakeCall(TEXT("MakeNoulQuestion"));
    auto* Criteria = NewObject<UK2Node_MakeStruct>(Graph);
    Criteria->StructType = FJWNU_SystemOneNoulCriteria::StaticStruct();
    Graph->AddNode(Criteria); Criteria->CreateNewGuid(); Criteria->AllocateDefaultPins();
    Schema->TrySetDefaultValue(*Criteria->FindPinChecked(TEXT("TrueDescription")), TEXT("이전 문의 있음"));
    Schema->TrySetDefaultValue(*Criteria->FindPinChecked(TEXT("FalseDescription")), TEXT("이전 문의 없음"));
    UEdGraphPin* CriteriaOutput = nullptr;
    for (auto* Pin : Criteria->Pins) { if (Pin->Direction == EGPD_Output) { CriteriaOutput = Pin; break; } }
    Test->TestTrue(TEXT("Noul criteria struct wire"), Schema->TryCreateConnection(CriteriaOutput, WithCriteria->FindPinChecked(TEXT("Criteria"))));
    Test->TestTrue(TEXT("Noul with criteria consumed"), Schema->TryCreateConnection(WithCriteria->GetReturnValuePin(), Record->FindPinChecked(TEXT("WithCriteria"))));
    Test->TestTrue(TEXT("Noul default criteria consumed"), Schema->TryCreateConnection(WithoutCriteria->GetReturnValuePin(), Record->FindPinChecked(TEXT("WithoutCriteria"))));
    FKismetEditorUtilities::CompileBlueprint(BP);
    Test->TestTrue(TEXT("SystemOne receiver BP compiled"), BP->Status != BS_Error);
    return BP;
}

}
