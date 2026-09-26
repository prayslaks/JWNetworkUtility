// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "JWNU_HttpRequest.h"
#include "JWNU_HttpTestReceiver.generated.h"

/** HTTP 요청의 실제 BP 실행·결과 전달을 기록하는 테스트 수신기다. */
UCLASS(Blueprintable)
class UJWNU_HttpTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    int32 CompletedCount = 0, FailedCount = 0, RetryCount = 0;
    FJWNU_HttpResult Result;
    FJWNU_HttpError Error;
    EJWNU_HttpStatusCode ImmediateStatus = EJWNU_HttpStatusCode::None;
    FString ImmediateBody;
    /** 즉시 HTTP 콜백을 실제 BP 이벤트로 넘기는 함수. */
    UFUNCTION() void ImmediateCallback(EJWNU_HttpStatusCode StatusCode, FString ResponseBody) { ImmediateResponse(StatusCode, ResponseBody); }
    /** 즉시 HTTP 응답을 BP 그래프에서 수신하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="HTTP Test") void ImmediateResponse(EJWNU_HttpStatusCode StatusCode, const FString& ResponseBody);
    virtual void ImmediateResponse_Implementation(EJWNU_HttpStatusCode StatusCode, const FString& ResponseBody) { RecordImmediate(StatusCode, ResponseBody); }
    /** 즉시 HTTP 응답 콜백을 기록하는 함수. */
    UFUNCTION(BlueprintCallable, Category="HTTP Test") void RecordImmediate(EJWNU_HttpStatusCode StatusCode, const FString& ResponseBody)
    {
        ImmediateStatus = StatusCode; ImmediateBody = ResponseBody;
        if (StatusCode == EJWNU_HttpStatusCode::OK || StatusCode == EJWNU_HttpStatusCode::OtherSuccess) { ++CompletedCount; }
        else { ++FailedCount; }
    }
    /** QueryParams 미연결 Start 노드를 실행하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="HTTP Test") void StartDefaults(UJWNU_HttpRequest* Target, const FString& URL);
    /** 실제 BP 그래프에 성공 응답을 전달하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="HTTP Test") void Completed(const FJWNU_HttpResult& Value);
    virtual void Completed_Implementation(const FJWNU_HttpResult& Value) { Record(Value); }
    /** BP 실행 결과를 기록하는 함수. */
    UFUNCTION(BlueprintCallable, Category="HTTP Test") void Record(const FJWNU_HttpResult& Value) { ++CompletedCount; Result = Value; }
    /** 실패 이벤트를 기록하는 함수. */
    UFUNCTION() void Failed(const FJWNU_HttpError& Value) { ++FailedCount; Error = Value; }
    /** 재시도 이벤트를 기록하는 함수. */
    UFUNCTION() void Retry(int32 AttemptNumber) { ++RetryCount; }
};
