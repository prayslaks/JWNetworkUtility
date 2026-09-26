// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_TypeSafeTypes.h"
#include "JWNU_TypeSafeTestReceiver.generated.h"

class UJWNU_TypeSafeRequest;

/** 자동 생성 BP 그래프를 통과한 Jev 결과를 기록한다. */
UCLASS(Blueprintable)
class UJWNU_TypeSafeTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    /** Options를 연결하지 않은 Start 노드의 실행을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="TypeSafe Test")
    void StartWithDefaultOptions(UJWNU_TypeSafeRequest* Request, const FJWNU_TypeSafeState& State, const TArray<FJWNU_TypeSafeQuestion>& Questions);
    /** Options를 연결하지 않은 환경변수 Start 노드의 컴파일을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="TypeSafe Test")
    void StartEnvironmentWithDefaultOptions(UJWNU_TypeSafeRequest* Request, const FJWNU_TypeSafeState& State, const TArray<FJWNU_TypeSafeQuestion>& Questions);
    /** Options·콜백 미연결 즉시 실행 BP 노드를 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="TypeSafe Test")
    void CallWithDefaultOptions(UObject* Context, const FJWNU_TypeSafeState& State, const TArray<FJWNU_TypeSafeQuestion>& Questions);
    UFUNCTION(BlueprintImplementableEvent, Category="TypeSafe Test")
    void CallEnvironmentWithDefaultOptions(UObject* Context, const FJWNU_TypeSafeState& State, const TArray<FJWNU_TypeSafeQuestion>& Questions);
    UFUNCTION(BlueprintCallable, Category="TypeSafe Test") void RecordStartedRequest(UJWNU_TypeSafeRequest* Request) { AuxiliaryRequest = Request; }
    UPROPERTY(Transient) TObjectPtr<UJWNU_TypeSafeRequest> AuxiliaryRequest;
    int32 CompletedCount = 0, FailedCount = 0;
    FJWNU_TypeSafeResult LastResult;
    FJWNU_TypeSafeError LastError;
    FJWNU_TypeSafeQuestion LastWithCriteria, LastWithoutCriteria;
    /** 성공 결과가 실제 BP 그래프를 통과하도록 제공하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="TypeSafe Test") void Completed(const FJWNU_TypeSafeResult& Result);
    virtual void Completed_Implementation(const FJWNU_TypeSafeResult& Result) { Record(Result, {}, {}); }
    /** BP가 전달한 결과를 기록하는 함수. */
    UFUNCTION(BlueprintCallable, Category="TypeSafe Test") void Record(const FJWNU_TypeSafeResult& Result, const FJWNU_TypeSafeQuestion& WithCriteria, const FJWNU_TypeSafeQuestion& WithoutCriteria)
    { check(IsInGameThread()); ++CompletedCount; LastResult = Result; LastWithCriteria = WithCriteria; LastWithoutCriteria = WithoutCriteria; }
    /** 동적 델리게이트의 오류 전달을 기록하는 함수. */
    UFUNCTION() void Failed(const FJWNU_TypeSafeError& Error)
    { check(IsInGameThread()); ++FailedCount; LastError = Error; }
};
