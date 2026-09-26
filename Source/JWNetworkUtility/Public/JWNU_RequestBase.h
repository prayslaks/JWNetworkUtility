// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_RequestBase.generated.h"

/** 전송 방식과 무관한 일회용 요청의 수명 상태다. */
UENUM(BlueprintType)
enum class EJWNU_RequestState : uint8 { Created, Active, Succeeded, Failed, Cancelled };

class UJWNU_RequestBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJWNU_RequestFinishedBP, UJWNU_RequestBase*, Request, EJWNU_RequestState, State);
DECLARE_MULTICAST_DELEGATE_TwoParams(FJWNU_RequestFinishedNative, UJWNU_RequestBase*, EJWNU_RequestState);

/** HTTP·API·SSE·공급자 요청을 BP 변수·배열·매크로에서 함께 제어하는 공통 부모다. */
UCLASS(Abstract, BlueprintType)
class JWNETWORKUTILITY_API UJWNU_RequestBase : public UObject
{
    GENERATED_BODY()
public:
    /** 활성 요청을 취소한다. 생성 전·종료 후 호출은 상태와 이벤트를 변경하지 않는다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|Request") void Cancel();
    /** 실행·재시도·인증 갱신·예약된 오류 전달을 기다리는지 반환한다. */
    UFUNCTION(BlueprintPure, Category="JWNU|Request") bool IsActive() const { return RequestState == EJWNU_RequestState::Active; }
    /** 이미 종료된 즉시 실행 요청도 조회할 수 있는 수명 상태다. */
    UFUNCTION(BlueprintPure, Category="JWNU|Request") EJWNU_RequestState GetState() const { return RequestState; }
    /** 타입별 결과 이벤트 이후 한 번 발생한다. 결과를 해석하려면 구체적인 요청 타입을 사용한다. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|Request") FJWNU_RequestFinishedBP OnFinished;
    FJWNU_RequestFinishedNative OnFinishedNative;
protected:
    virtual void CancelRequest() PURE_VIRTUAL(UJWNU_RequestBase::CancelRequest, );
    bool ActivateRequest();
    bool SetFinishedState(EJWNU_RequestState TerminalState);
    void BroadcastFinished();
private:
    /** 부모가 단독 소유하는 일회용 요청 상태다. */
    UPROPERTY(Transient) EJWNU_RequestState RequestState = EJWNU_RequestState::Created;
    bool bFinishedBroadcast = false;
};
