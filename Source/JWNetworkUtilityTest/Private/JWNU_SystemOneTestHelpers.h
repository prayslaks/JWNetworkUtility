// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "JWNU_SystemOneTestReceiver.h"
#include "JWNU_SystemOneRequest.h"
#include "Misc/AutomationTest.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"

namespace JWNU::SystemOneTest
{
inline UBlueprint* BuildReceiver(FAutomationTestBase* Test)
{
    auto* BP = FKismetEditorUtilities::CreateBlueprint(UJWNU_SystemOneTestReceiver::StaticClass(), GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("BP_SystemOneAutomation")),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("ReceiveGraph"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(BP, Graph);
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    for (bool bStart : {false, true})
    {
        auto* Event = NewObject<UK2Node_Event>(Graph);
        Event->EventReference.SetExternalMember(bStart ? TEXT("StartDefaults") : TEXT("Completed"), UJWNU_SystemOneTestReceiver::StaticClass());
        Event->bOverrideFunction = true; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
        auto* Call = NewObject<UK2Node_CallFunction>(Graph);
        Call->SetFromFunction(bStart ? UJWNU_SystemOneRequest::StaticClass()->FindFunctionByName(TEXT("Start"))
            : UJWNU_SystemOneTestReceiver::StaticClass()->FindFunctionByName(TEXT("Record")));
        Graph->AddNode(Call); Call->CreateNewGuid(); Call->AllocateDefaultPins();
        Test->TestTrue(TEXT("BP exec wire"), Schema->TryCreateConnection(Event->FindPinChecked(UEdGraphSchema_K2::PN_Then), Call->FindPinChecked(UEdGraphSchema_K2::PN_Execute)));
        if (bStart)
        {
            Test->TestTrue(TEXT("BP request target"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Request")), Call->FindPinChecked(UEdGraphSchema_K2::PN_Self)));
            for (const TCHAR* Pin : {TEXT("State"), TEXT("Questions")})
            { Test->TestTrue(TEXT("BP required input"), Schema->TryCreateConnection(Event->FindPinChecked(Pin), Call->FindPinChecked(Pin))); }
            Test->TestFalse(TEXT("Default Options is generated"), Call->FindPinChecked(TEXT("Options"))->bDefaultValueIsIgnored);
        }
        else { Test->TestTrue(TEXT("BP typed result"), Schema->TryCreateConnection(Event->FindPinChecked(TEXT("Result")), Call->FindPinChecked(TEXT("Result")))); }
    }
    FKismetEditorUtilities::CompileBlueprint(BP);
    Test->TestTrue(TEXT("System One BP compiles"), BP->Status != BS_Error);
    return BP;
}

}
