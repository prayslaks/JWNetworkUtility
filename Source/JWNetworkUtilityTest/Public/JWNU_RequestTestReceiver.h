// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_RequestBase.h"
#include "JWNU_RequestTestReceiver.generated.h"

/** 공통 요청 타입의 BP 실행·종료 알림을 검증하는 수신기다. */
UCLASS(Blueprintable)
class UJWNU_RequestTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    /** 생성한 BP 그래프에서 요청 Cancel 호출을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Request Test") void CancelInBlueprint(UJWNU_RequestBase* Request);
    /** 종료 알림이 BP 그래프에 전달되는지 검증하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="Request Test") void Finished(UJWNU_RequestBase* Request, EJWNU_RequestState State);
    virtual void Finished_Implementation(UJWNU_RequestBase* Request, EJWNU_RequestState State) { Record(Request, State); }
    /** 종료 결과를 기록하고 콜백 안에서 재취소하여 중복 완료 방지를 검증하는 함수. */
    UFUNCTION(BlueprintCallable, Category="Request Test") void Record(UJWNU_RequestBase* Request, EJWNU_RequestState State)
    { ++Count; LastRequest = Request; LastState = State; Request->Cancel(); }
    /** 마지막 종료 이벤트의 요청 객체를 GC로부터 보관하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_RequestBase> LastRequest;
    int32 Count = 0;
    EJWNU_RequestState LastState = EJWNU_RequestState::Created;
};
