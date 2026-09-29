// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_SystemOneTypes.h"
#include "JWNU_BFL_SystemOne.generated.h"

class UJWNU_SystemOneRequest;
DECLARE_DYNAMIC_DELEGATE_OneParam(FJWNU_SystemOneCompletedCallback, const FJWNU_SystemOneResult&, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FJWNU_SystemOneFailedCallback, const FJWNU_SystemOneError&, Error);

/** System One의 질문·연결 옵션 생성과 콜백 기반 즉시 호출 노드를 제공한다. */
UCLASS()
class JWNETWORKUTILITYSYSTEMONE_API UJWNU_BFL_SystemOne : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** OpenRouter 공식 Decisions 주소와 지정 모델의 기본 옵션을 만드는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|SystemOne")
    static FJWNU_SystemOneOptions MakeOpenRouterOptions(const FString& Model = TEXT("~typesafe/jev-latest"));
    /** 공통 요청에 연결할 TypeSafe 직접 호출 옵션을 만드는 함수. 기본 OpenRouter 옵션을 대체한다. */
    UFUNCTION(BlueprintPure, Category="JWNU|SystemOne")
    static FJWNU_SystemOneOptions MakeTypeSafeOptions(const FString& Model = TEXT("jev-latest"));
    /** 선택을 돕는 모델 ID 목록을 반환하는 함수. 다른 문자열 모델 ID도 사용할 수 있다. */
    UFUNCTION(BlueprintPure, Category="JWNU|SystemOne")
    static TArray<FString> GetOpenRouterModelPresets();
    /** 콜백을 연결하고 평가를 예약하는 함수. API Key는 직접 전달하며 반환 요청은 취소·조회용이다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SystemOne", meta=(WorldContext="WorldContextObject", DisplayName="Call System One API", AutoCreateRefTerm="Options,OnCompleted,OnFailed"))
    static UJWNU_SystemOneRequest* CallSystemOneApi(const UObject* WorldContextObject, const FJWNU_SystemOneState& State,
        const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options,
        UPARAM(DisplayName="API Key") const FString& ApiKey, const FJWNU_SystemOneCompletedCallback& OnCompleted, const FJWNU_SystemOneFailedCallback& OnFailed);
    /** 공식 endpoint에 맞는 환경변수 키로 평가를 예약하는 함수. Options 미연결 시 OpenRouter 기본값을 사용한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SystemOne", meta=(WorldContext="WorldContextObject", DisplayName="Call System One API From Environment", AutoCreateRefTerm="Options,OnCompleted,OnFailed"))
    static UJWNU_SystemOneRequest* CallSystemOneApiFromEnvironment(const UObject* WorldContextObject, const FJWNU_SystemOneState& State,
        const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options,
        const FJWNU_SystemOneCompletedCallback& OnCompleted, const FJWNU_SystemOneFailedCallback& OnFailed);
    /** Choice 질문을 만드는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|SystemOne")
    static FJWNU_SystemOneQuestion MakeChoiceQuestion(const FString& Id, const FString& Instructions, const TMap<FString, FString>& Criteria);
    /** Score 질문을 만드는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|SystemOne")
    static FJWNU_SystemOneQuestion MakeScoreQuestion(const FString& Id, const FString& Instructions, const TArray<FString>& Criteria);
    /** 선택적 true/false 설명을 포함한 Noul 질문을 만드는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|SystemOne", meta=(AutoCreateRefTerm="Criteria"))
    static FJWNU_SystemOneQuestion MakeNoulQuestion(const FString& Id, const FString& Instructions, const FJWNU_SystemOneNoulCriteria& Criteria);
};
