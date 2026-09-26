// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_BFL_TypeSafe.h"
#include "JWNU_TypeSafeRequest.h"
#include "UObject/StrongObjectPtr.h"

namespace JWNU::TypeSafeBFL
{
inline UJWNU_TypeSafeRequest* CreateBoundRequest(const UObject* WorldContextObject,
    const FJWNU_TypeSafeCompletedCallback& OnCompleted, const FJWNU_TypeSafeFailedCallback& OnFailed)
{
    auto* Request = UJWNU_TypeSafeRequest::CreateTypeSafeRequest(WorldContextObject);
    if (!Request)
    {
        FJWNU_TypeSafeError Error; Error.Code = EJWNU_TypeSafeErrorCode::Configuration;
        Error.Message = TEXT("Cannot create TypeSafe request for this world.");
        OnFailed.ExecuteIfBound(Error);
        return nullptr;
    }
    Request->OnCompletedNative.AddLambda([OnCompleted](const FJWNU_TypeSafeResult& Result) { OnCompleted.ExecuteIfBound(Result); });
    Request->OnFailedNative.AddLambda([OnFailed](const FJWNU_TypeSafeError& Error) { OnFailed.ExecuteIfBound(Error); });
    return Request;
}
}

UJWNU_TypeSafeRequest* UJWNU_BFL_TypeSafe::CallTypeSafeApi(const UObject* WorldContextObject, const FJWNU_TypeSafeState& State,
    const TArray<FJWNU_TypeSafeQuestion>& Questions, const FJWNU_TypeSafeOptions& Options, const FString& ApiKey,
    const FJWNU_TypeSafeCompletedCallback& OnCompleted, const FJWNU_TypeSafeFailedCallback& OnFailed)
{
    TStrongObjectPtr<UJWNU_TypeSafeRequest> Request(JWNU::TypeSafeBFL::CreateBoundRequest(WorldContextObject, OnCompleted, OnFailed));
    // Start=false도 입력 오류 통지를 예약할 수 있으므로 실패 콜백을 여기서 중복 발생시키지 않는다.
    if (Request.IsValid()) { Request->Start(State, Questions, Options, ApiKey); }
    return Request.Get();
}

UJWNU_TypeSafeRequest* UJWNU_BFL_TypeSafe::CallTypeSafeApiFromEnvironment(const UObject* WorldContextObject, const FJWNU_TypeSafeState& State,
    const TArray<FJWNU_TypeSafeQuestion>& Questions, const FJWNU_TypeSafeOptions& Options,
    const FJWNU_TypeSafeCompletedCallback& OnCompleted, const FJWNU_TypeSafeFailedCallback& OnFailed)
{
    TStrongObjectPtr<UJWNU_TypeSafeRequest> Request(JWNU::TypeSafeBFL::CreateBoundRequest(WorldContextObject, OnCompleted, OnFailed));
    if (Request.IsValid()) { Request->StartFromEnvironment(State, Questions, Options); }
    return Request.Get();
}

FJWNU_TypeSafeQuestion UJWNU_BFL_TypeSafe::MakeChoiceQuestion(const FString& Id, const FString& Instructions, const TMap<FString, FString>& Criteria)
{
    FJWNU_TypeSafeQuestion Q; Q.Id = Id; Q.Instructions = Instructions;
    Q.Type = EJWNU_TypeSafeQuestionType::Choice; Q.ChoiceOptions = Criteria; return Q;
}
FJWNU_TypeSafeQuestion UJWNU_BFL_TypeSafe::MakeScoreQuestion(const FString& Id, const FString& Instructions, const TArray<FString>& Criteria)
{
    FJWNU_TypeSafeQuestion Q; Q.Id = Id; Q.Instructions = Instructions;
    Q.Type = EJWNU_TypeSafeQuestionType::Score; Q.ScoreLevels = Criteria; return Q;
}
FJWNU_TypeSafeQuestion UJWNU_BFL_TypeSafe::MakeNoulQuestion(const FString& Id, const FString& Instructions, const FJWNU_TypeSafeNoulCriteria& Criteria)
{
    FJWNU_TypeSafeQuestion Q; Q.Id = Id; Q.Instructions = Instructions;
    Q.Type = EJWNU_TypeSafeQuestionType::Noul;
    Q.YesDescription = Criteria.TrueDescription; Q.NoDescription = Criteria.FalseDescription; return Q;
}
