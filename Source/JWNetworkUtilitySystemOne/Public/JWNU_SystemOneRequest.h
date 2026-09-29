// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_RequestBase.h"
#include "JWNU_SystemOneTypes.h"
#include "JWNU_SystemOneRequest.generated.h"

class UJWNU_GIS_SystemOne;
class UJWNU_JsonHttpJob;
class UWorld;
struct FJWNU_JsonHttpResult;

/** System One 평가 한 건의 요청·응답 수명을 관리하는 일회용 핸들이다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITYSYSTEMONE_API UJWNU_SystemOneRequest : public UJWNU_RequestBase
{
    GENERATED_BODY()
public:
    /** 전송 없이 일회용 핸들을 생성하는 함수. 변수에 보관하고 OnCompleted·OnFailed를 바인딩한 뒤 Start한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SystemOne", meta=(WorldContext="WorldContextObject", DisplayName="Create System One Request"))
    static UJWNU_SystemOneRequest* CreateSystemOneRequest(const UObject* WorldContextObject);
    /** 이벤트 바인딩 후 명시적 키로 평가를 예약하는 함수. Options 기본값은 OpenRouter이며 입력 오류·응답은 다음 Pump부터 전달한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SystemOne", meta=(AutoCreateRefTerm="Options"))
    bool Start(const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options, UPARAM(DisplayName="API Key") const FString& ApiKey);
    /** 공식 URL에 맞는 OPENROUTER_API_KEY 또는 TYPESAFE_API_KEY로 평가를 예약하는 함수. TypeSafe 직접 호출은 MakeTypeSafeOptions를 연결한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SystemOne", meta=(AutoCreateRefTerm="Options"))
    bool StartFromEnvironment(const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options);
    /** 마지막 성공 결과를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|SystemOne") FJWNU_SystemOneResult GetResult() const { return Result; }
    /** 마지막 오류를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|SystemOne") FJWNU_SystemOneError GetError() const { return Error; }
    /** 성공 시 한 번 발생하는 BP 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|SystemOne") FJWNU_SystemOneCompletedBP OnCompleted;
    /** 실패·취소 시 한 번 발생하는 BP 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|SystemOne") FJWNU_SystemOneFailedBP OnFailed;
    FJWNU_SystemOneCompletedNative OnCompletedNative;
    FJWNU_SystemOneFailedNative OnFailedNative;
private:
    virtual void CancelRequest() override;
    friend class UJWNU_GIS_SystemOne;
    bool Schedule(const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options, const FString& ApiKey, const FString& ApiKeyError);
    void Pump();
    void Receive(const FJWNU_JsonHttpResult& Response);
    void Finish();
    /** 진행 중인 전송을 GC로부터 보호하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_JsonHttpJob> Job;
    /** 마지막 성공 결과를 보존하는 필드. */
    UPROPERTY(Transient) FJWNU_SystemOneResult Result;
    /** 마지막 실패 진단을 보존하는 필드. */
    UPROPERTY(Transient) FJWNU_SystemOneError Error;
    TWeakObjectPtr<UJWNU_GIS_SystemOne> OwnerClient;
    TWeakObjectPtr<UWorld> OwnerWorld;
    TArray<FJWNU_SystemOneQuestion> SubmittedQuestions;
    bool bStarted = false;
};
