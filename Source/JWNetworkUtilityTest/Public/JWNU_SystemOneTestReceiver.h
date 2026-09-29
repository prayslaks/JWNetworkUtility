// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_SystemOneTypes.h"
#include "JWNU_SystemOneTestReceiver.generated.h"

class UJWNU_SystemOneRequest;

/** 공통 판단 결과가 실제 BP 그래프를 통과하는지 검증하는 수신 객체다. */
UCLASS(Blueprintable)
class UJWNU_SystemOneTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    /** 기본 옵션 Start 노드를 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent) void StartDefaults(UJWNU_SystemOneRequest* Request, const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions);
    /** 성공 결과를 BP 그래프에 전달하는 함수. */
    UFUNCTION(BlueprintNativeEvent) void Completed(const FJWNU_SystemOneResult& Result);
    virtual void Completed_Implementation(const FJWNU_SystemOneResult& Result) { Record(Result); }
    /** BP 그래프를 통과한 결과를 기록하는 함수. */
    UFUNCTION(BlueprintCallable) void Record(const FJWNU_SystemOneResult& Result) { check(IsInGameThread()); ++CompletedCount; LastResult = Result; }
    /** 실패 결과를 기록하는 함수. */
    UFUNCTION() void Failed(const FJWNU_SystemOneError& Error) { check(IsInGameThread()); ++FailedCount; LastError = Error; }
    int32 CompletedCount = 0, FailedCount = 0;
    FJWNU_SystemOneResult LastResult;
    FJWNU_SystemOneError LastError;
};
