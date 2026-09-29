// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_SystemOneTypes.h"
#include "JWNU_SystemOneTransportTestReceiver.generated.h"

class UJWNU_SystemOneRequest;

/** 자동 생성 BP 그래프를 통과한 Jev 결과를 기록한다. */
UCLASS(Blueprintable)
class UJWNU_SystemOneTransportTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    /** Options를 연결하지 않은 Start 노드의 실행을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="SystemOne Test")
    void StartWithDefaultOptions(UJWNU_SystemOneRequest* Request, const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions);
    /** Options를 연결하지 않은 환경변수 Start 노드의 컴파일을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="SystemOne Test")
    void StartEnvironmentWithDefaultOptions(UJWNU_SystemOneRequest* Request, const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions);
    /** Options·콜백 미연결 즉시 실행 BP 노드를 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="SystemOne Test")
    void CallWithDefaultOptions(UObject* Context, const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions);
    /** Options·콜백 미연결 환경변수 즉시 호출 노드의 BP 컴파일을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="SystemOne Test")
    void CallEnvironmentWithDefaultOptions(UObject* Context, const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions);
    /** 즉시 호출 노드가 반환한 요청을 취소·조회 검증용으로 보관하는 함수. */
    UFUNCTION(BlueprintCallable, Category="SystemOne Test") void RecordStartedRequest(UJWNU_SystemOneRequest* Request) { AuxiliaryRequest = Request; }
    /** BP 즉시 호출이 생성한 요청의 GC 수명을 유지하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_SystemOneRequest> AuxiliaryRequest;
    int32 CompletedCount = 0, FailedCount = 0;
    FJWNU_SystemOneResult LastResult;
    FJWNU_SystemOneError LastError;
    FJWNU_SystemOneQuestion LastWithCriteria, LastWithoutCriteria;
    /** 성공 결과가 실제 BP 그래프를 통과하도록 제공하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="SystemOne Test") void Completed(const FJWNU_SystemOneResult& Result);
    virtual void Completed_Implementation(const FJWNU_SystemOneResult& Result) { Record(Result, {}, {}); }
    /** BP가 전달한 결과를 기록하는 함수. */
    UFUNCTION(BlueprintCallable, Category="SystemOne Test") void Record(const FJWNU_SystemOneResult& Result, const FJWNU_SystemOneQuestion& WithCriteria, const FJWNU_SystemOneQuestion& WithoutCriteria)
    { check(IsInGameThread()); ++CompletedCount; LastResult = Result; LastWithCriteria = WithCriteria; LastWithoutCriteria = WithoutCriteria; }
    /** 동적 델리게이트의 오류 전달을 기록하는 함수. */
    UFUNCTION() void Failed(const FJWNU_SystemOneError& Error)
    { check(IsInGameThread()); ++FailedCount; LastError = Error; }
};
