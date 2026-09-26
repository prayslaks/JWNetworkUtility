// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_RequestBase.h"
#include "JWNU_TypeSafeTypes.h"
#include "JWNU_TypeSafeRequest.generated.h"

class UJWNU_GIS_TypeSafe;
class UJWNU_JsonHttpJob;
class UWorld;
struct FJWNU_JsonHttpResult;

/** Jev 평가 한 건의 요청·응답 수명을 관리하는 일회용 핸들이다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITYTYPESAFE_API UJWNU_TypeSafeRequest : public UJWNU_RequestBase
{
    GENERATED_BODY()
public:
    /** 전송 없이 일회용 핸들을 생성하는 함수. 변수에 보관하고 OnCompleted·OnFailed를 바인딩한 뒤 Start한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|TypeSafe", meta=(WorldContext="WorldContextObject", DisplayName="Create TypeSafe Request"))
    static UJWNU_TypeSafeRequest* CreateTypeSafeRequest(const UObject* WorldContextObject);
    /** 이벤트 바인딩 후 평가를 한 번 예약하는 함수. 미연결 Options는 기본값을 사용하며 결과는 다음 Pump부터 전달한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|TypeSafe", meta=(AutoCreateRefTerm="Options"))
    bool Start(const FJWNU_TypeSafeState& State, const TArray<FJWNU_TypeSafeQuestion>& Questions, const FJWNU_TypeSafeOptions& Options, UPARAM(DisplayName="API Key") const FString& ApiKey);
    /** 공식 URL에 TYPESAFE_API_KEY로 평가를 한 번 예약하는 함수. 미연결 Options는 기본값이며 Start와 둘 중 하나만 호출한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|TypeSafe", meta=(AutoCreateRefTerm="Options"))
    bool StartFromEnvironment(const FJWNU_TypeSafeState& State, const TArray<FJWNU_TypeSafeQuestion>& Questions, const FJWNU_TypeSafeOptions& Options);
    /** 마지막 성공 결과를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|TypeSafe") FJWNU_TypeSafeResult GetResult() const { return Result; }
    /** 마지막 오류를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|TypeSafe") FJWNU_TypeSafeError GetError() const { return Error; }
    /** 성공 시 한 번 발생하는 BP 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|TypeSafe") FJWNU_TypeSafeCompletedBP OnCompleted;
    /** 실패·취소 시 한 번 발생하는 BP 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|TypeSafe") FJWNU_TypeSafeFailedBP OnFailed;
    FJWNU_TypeSafeCompletedNative OnCompletedNative;
    FJWNU_TypeSafeFailedNative OnFailedNative;
private:
    virtual void CancelRequest() override;
    friend class UJWNU_GIS_TypeSafe;
    bool Schedule(const FJWNU_TypeSafeState& State, const TArray<FJWNU_TypeSafeQuestion>& Questions, const FJWNU_TypeSafeOptions& Options, const FString& ApiKey, const FString& ApiKeyError);
    void Pump();
    void Receive(const FJWNU_JsonHttpResult& Response);
    void Finish();
    /** 진행 중인 전송을 GC로부터 보호하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_JsonHttpJob> Job;
    /** 마지막 성공 결과를 보존하는 필드. */
    UPROPERTY(Transient) FJWNU_TypeSafeResult Result;
    /** 마지막 실패 진단을 보존하는 필드. */
    UPROPERTY(Transient) FJWNU_TypeSafeError Error;
    TWeakObjectPtr<UJWNU_GIS_TypeSafe> OwnerClient;
    TWeakObjectPtr<UWorld> OwnerWorld;
    TArray<FJWNU_TypeSafeQuestion> SubmittedQuestions;
    bool bStarted = false;
};
