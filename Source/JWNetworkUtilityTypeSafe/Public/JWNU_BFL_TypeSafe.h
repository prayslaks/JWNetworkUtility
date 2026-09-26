// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_TypeSafeTypes.h"
#include "JWNU_BFL_TypeSafe.generated.h"

class UJWNU_TypeSafeRequest;
DECLARE_DYNAMIC_DELEGATE_OneParam(FJWNU_TypeSafeCompletedCallback, const FJWNU_TypeSafeResult&, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FJWNU_TypeSafeFailedCallback, const FJWNU_TypeSafeError&, Error);

/** Jev의 Choice·Score·Noul 질문 생성 노드를 제공한다. */
UCLASS()
class JWNETWORKUTILITYTYPESAFE_API UJWNU_BFL_TypeSafe : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** 콜백 연결 후 평가를 예약한다. 반환 요청은 선택적 취소·조회용이다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|TypeSafe", meta=(WorldContext="WorldContextObject", DisplayName="Call TypeSafe API", AutoCreateRefTerm="Options,OnCompleted,OnFailed"))
    static UJWNU_TypeSafeRequest* CallTypeSafeApi(const UObject* WorldContextObject, const FJWNU_TypeSafeState& State,
        const TArray<FJWNU_TypeSafeQuestion>& Questions, const FJWNU_TypeSafeOptions& Options,
        UPARAM(DisplayName="API Key") const FString& ApiKey, const FJWNU_TypeSafeCompletedCallback& OnCompleted, const FJWNU_TypeSafeFailedCallback& OnFailed);
    /** 환경변수 키로 콜백 연결 후 평가를 예약한다. Options 미연결 시 기본값을 사용한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|TypeSafe", meta=(WorldContext="WorldContextObject", DisplayName="Call TypeSafe API From Environment", AutoCreateRefTerm="Options,OnCompleted,OnFailed"))
    static UJWNU_TypeSafeRequest* CallTypeSafeApiFromEnvironment(const UObject* WorldContextObject, const FJWNU_TypeSafeState& State,
        const TArray<FJWNU_TypeSafeQuestion>& Questions, const FJWNU_TypeSafeOptions& Options,
        const FJWNU_TypeSafeCompletedCallback& OnCompleted, const FJWNU_TypeSafeFailedCallback& OnFailed);
    /** Choice 질문을 만드는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|TypeSafe")
    static FJWNU_TypeSafeQuestion MakeChoiceQuestion(const FString& Id, const FString& Instructions, const TMap<FString, FString>& Criteria);
    /** Score 질문을 만드는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|TypeSafe")
    static FJWNU_TypeSafeQuestion MakeScoreQuestion(const FString& Id, const FString& Instructions, const TArray<FString>& Criteria);
    /** 선택적 true/false 설명을 포함한 Noul 질문을 만드는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|TypeSafe", meta=(AutoCreateRefTerm="Criteria"))
    static FJWNU_TypeSafeQuestion MakeNoulQuestion(const FString& Id, const FString& Instructions, const FJWNU_TypeSafeNoulCriteria& Criteria);
};
