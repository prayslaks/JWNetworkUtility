// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

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
    UFUNCTION(BlueprintImplementableEvent, Category="Request Test") void CancelInBlueprint(UJWNU_RequestBase* Request);
    UFUNCTION(BlueprintNativeEvent, Category="Request Test") void Finished(UJWNU_RequestBase* Request, EJWNU_RequestState State);
    virtual void Finished_Implementation(UJWNU_RequestBase* Request, EJWNU_RequestState State) { Record(Request, State); }
    UFUNCTION(BlueprintCallable, Category="Request Test") void Record(UJWNU_RequestBase* Request, EJWNU_RequestState State)
    { ++Count; LastRequest = Request; LastState = State; Request->Cancel(); }
    UPROPERTY(Transient) TObjectPtr<UJWNU_RequestBase> LastRequest;
    int32 Count = 0;
    EJWNU_RequestState LastState = EJWNU_RequestState::Created;
};
