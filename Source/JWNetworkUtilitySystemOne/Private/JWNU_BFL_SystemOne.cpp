// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_BFL_SystemOne.h"
#include "JWNU_SystemOneRequest.h"
#include "UObject/StrongObjectPtr.h"

#include "JWNU_SystemOneBFLHelpers.h"

UJWNU_SystemOneRequest* UJWNU_BFL_SystemOne::CallSystemOneApi(const UObject* WorldContextObject, const FJWNU_SystemOneState& State,
    const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options, const FString& ApiKey,
    const FJWNU_SystemOneCompletedCallback& OnCompleted, const FJWNU_SystemOneFailedCallback& OnFailed)
{
    TStrongObjectPtr<UJWNU_SystemOneRequest> Request(JWNU::SystemOneBFL::CreateBoundRequest(WorldContextObject, OnCompleted, OnFailed));
    // Start=false도 입력 오류 통지를 예약할 수 있으므로 실패 콜백을 여기서 중복 발생시키지 않는다.
    if (Request.IsValid()) { Request->Start(State, Questions, Options, ApiKey); }
    return Request.Get();
}

UJWNU_SystemOneRequest* UJWNU_BFL_SystemOne::CallSystemOneApiFromEnvironment(const UObject* WorldContextObject, const FJWNU_SystemOneState& State,
    const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options,
    const FJWNU_SystemOneCompletedCallback& OnCompleted, const FJWNU_SystemOneFailedCallback& OnFailed)
{
    TStrongObjectPtr<UJWNU_SystemOneRequest> Request(JWNU::SystemOneBFL::CreateBoundRequest(WorldContextObject, OnCompleted, OnFailed));
    if (Request.IsValid()) { Request->StartFromEnvironment(State, Questions, Options); }
    return Request.Get();
}

FJWNU_SystemOneQuestion UJWNU_BFL_SystemOne::MakeChoiceQuestion(const FString& Id, const FString& Instructions, const TMap<FString, FString>& Criteria)
{
    FJWNU_SystemOneQuestion Q; Q.Id = Id; Q.Instructions = Instructions;
    Q.Type = EJWNU_SystemOneQuestionType::Choice; Q.ChoiceOptions = Criteria; return Q;
}
FJWNU_SystemOneQuestion UJWNU_BFL_SystemOne::MakeScoreQuestion(const FString& Id, const FString& Instructions, const TArray<FString>& Criteria)
{
    FJWNU_SystemOneQuestion Q; Q.Id = Id; Q.Instructions = Instructions;
    Q.Type = EJWNU_SystemOneQuestionType::Score; Q.ScoreLevels = Criteria; return Q;
}
FJWNU_SystemOneQuestion UJWNU_BFL_SystemOne::MakeNoulQuestion(const FString& Id, const FString& Instructions, const FJWNU_SystemOneNoulCriteria& Criteria)
{
    FJWNU_SystemOneQuestion Q; Q.Id = Id; Q.Instructions = Instructions;
    Q.Type = EJWNU_SystemOneQuestionType::Noul;
    Q.YesDescription = Criteria.TrueDescription; Q.NoDescription = Criteria.FalseDescription; return Q;
}

FJWNU_SystemOneOptions UJWNU_BFL_SystemOne::MakeOpenRouterOptions(const FString& Model)
{
    FJWNU_SystemOneOptions Options; Options.Model = Model; return Options;
}
FJWNU_SystemOneOptions UJWNU_BFL_SystemOne::MakeTypeSafeOptions(const FString& Model)
{
    FJWNU_SystemOneOptions Options; Options.Endpoint = TEXT("https://api.typesafe.ai/v1/systemone"); Options.Model = Model; return Options;
}
TArray<FString> UJWNU_BFL_SystemOne::GetOpenRouterModelPresets()
{
    return {TEXT("~typesafe/jev-latest"), TEXT("respan/span-01"), TEXT("upstage/solar-decide")};
}
