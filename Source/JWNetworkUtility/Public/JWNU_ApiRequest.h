// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "JWNU_RequestBase.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNU_ApiRequest.generated.h"

class UJWNU_GIS_ApiClientService;
class UJWNU_HttpRequestJobHandle;
class UWorld;

/** 일반 API 요청의 종료 오류 분류다. */
UENUM(BlueprintType)
enum class EJWNU_ApiRequestError : uint8 { None, RequestFailed, Cancelled };

/** 기존 Call API의 상태 코드와 정규화된 응답 본문이다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_ApiResult
{
    GENERATED_BODY()
    /** 기존 API 계층의 HTTP 상태 코드 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|API") EJWNU_HttpStatusCode StatusCode = EJWNU_HttpStatusCode::None;
    /** 성공 응답 또는 기존 계층이 정규화한 오류 JSON 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|API") FString ResponseBody;
};

/** API 요청 실패와 취소의 진단 데이터다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_ApiError
{
    GENERATED_BODY()
    /** 요청 실패와 사용자 취소를 구분하는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|API") EJWNU_ApiRequestError Code = EJWNU_ApiRequestError::None;
    /** 실패 응답의 상태 코드와 본문 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|API") FJWNU_ApiResult Response;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_ApiCompletedBP, const FJWNU_ApiResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_ApiFailedBP, const FJWNU_ApiError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_ApiRetryBP, int32, AttemptNumber);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_ApiCompletedNative, const FJWNU_ApiResult&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_ApiFailedNative, const FJWNU_ApiError&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_ApiRetryNative, int32);

/** 생성·바인딩·시작 순서로 사용하는 일회용 API 요청이다. 기존 Host·JWT·재시도 계층을 사용한다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITY_API UJWNU_ApiRequest : public UJWNU_RequestBase
{
    GENERATED_BODY()
public:
    /** 전송 없이 핸들을 생성하는 함수. 변수에 저장하고 이벤트를 바인딩한 뒤 Start한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|API", meta=(WorldContext="WorldContextObject", DisplayName="Create API Request"))
    static UJWNU_ApiRequest* CreateApiRequest(const UObject* WorldContextObject);
    /** 바인딩 후 Host·JWT 설정으로 요청을 한 번 시작하는 함수. 즉시 오류도 이벤트로 전달한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|API", meta=(AutoCreateRefTerm="QueryParams"))
    bool Start(EJWNU_ServiceType ServiceType, EJWNU_HttpMethod Method, const FString& Endpoint, const FString& ContentBody, const TMap<FString, FString>& QueryParams, bool bRequiresAuth = true);
    /** 마지막 성공 응답을 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|API") FJWNU_ApiResult GetResult() const { return Result; }
    /** 마지막 오류를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|API") FJWNU_ApiError GetError() const { return Error; }
    /** HTTP 성공 응답을 전달하는 필드. 본문의 업무 성공 여부는 호출자가 판단한다. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|API") FJWNU_ApiCompletedBP OnCompleted;
    /** HTTP·설정·전송 실패 또는 취소를 한 번 전달하는 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|API") FJWNU_ApiFailedBP OnFailed;
    /** 전송 계층의 다음 시도 번호를 전달하는 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|API") FJWNU_ApiRetryBP OnRetry;
    FJWNU_ApiCompletedNative OnCompletedNative;
    FJWNU_ApiFailedNative OnFailedNative;
    FJWNU_ApiRetryNative OnRetryNative;
private:
    virtual void CancelRequest() override;
    friend class UJWNU_GIS_ApiClientService;
    void Receive(EJWNU_HttpStatusCode Status, const FString& Body);
    void Retry(int32 AttemptNumber);
    void Finish();
    /** 인증 갱신으로 내부 Job이 바뀌어도 유지되는 제어 핸들 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_HttpRequestJobHandle> Handle;
    /** 마지막 성공 응답 필드. */
    UPROPERTY(Transient) FJWNU_ApiResult Result;
    /** 마지막 실패 진단 필드. */
    UPROPERTY(Transient) FJWNU_ApiError Error;
    TWeakObjectPtr<UJWNU_GIS_ApiClientService> OwnerClient;
    TWeakObjectPtr<UWorld> OwnerWorld;
};
