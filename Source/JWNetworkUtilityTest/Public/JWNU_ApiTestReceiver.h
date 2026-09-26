// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "JWNU_ApiRequest.h"
#include "JWNU_ApiTestReceiver.generated.h"

/** API 요청의 BP 이벤트 전달을 기록하는 테스트 수신기다. */
UCLASS(Blueprintable)
class UJWNU_ApiTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    int32 CompletedCount = 0, FailedCount = 0;
    FJWNU_ApiResult Result;
    FJWNU_ApiError Error;
    /** 즉시 호출의 기존 두 인자 콜백을 BP에서 수신하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="API Test") void ImmediateResponse(EJWNU_HttpStatusCode StatusCode, const FString& ResponseBody);
    virtual void ImmediateResponse_Implementation(EJWNU_HttpStatusCode StatusCode, const FString& ResponseBody) { RecordImmediate(StatusCode, ResponseBody); }
    /** 값 전달 델리게이트를 BP 이벤트로 넘기는 함수. */
    UFUNCTION() void ImmediateCallback(EJWNU_HttpStatusCode StatusCode, FString ResponseBody) { ImmediateResponse(StatusCode, ResponseBody); }
    /** 즉시 호출 BP 콜백을 기존 검증용 결과로 기록하는 함수. */
    UFUNCTION(BlueprintCallable, Category="API Test") void RecordImmediate(EJWNU_HttpStatusCode StatusCode, FString ResponseBody)
    {
        if (StatusCode == EJWNU_HttpStatusCode::OK || StatusCode == EJWNU_HttpStatusCode::OtherSuccess)
        { ++CompletedCount; Result.StatusCode = StatusCode; Result.ResponseBody = ResponseBody; }
        else
        {
            ++FailedCount; Error.Response.StatusCode = StatusCode; Error.Response.ResponseBody = ResponseBody;
            Error.Code = ResponseBody.Contains(TEXT("CANCELLED")) ? EJWNU_ApiRequestError::Cancelled : EJWNU_ApiRequestError::RequestFailed;
        }
    }
    /** 실제 BP 그래프로 성공 응답을 전달하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="API Test") void Completed(const FJWNU_ApiResult& Value);
    virtual void Completed_Implementation(const FJWNU_ApiResult& Value) { Record(Value); }
    /** BP 실행 결과를 기록하는 함수. */
    UFUNCTION(BlueprintCallable, Category="API Test") void Record(const FJWNU_ApiResult& Value) { ++CompletedCount; Result = Value; }
    /** 바인딩된 실패 이벤트를 기록하는 함수. */
    UFUNCTION() void Failed(const FJWNU_ApiError& Value) { ++FailedCount; Error = Value; }
};
