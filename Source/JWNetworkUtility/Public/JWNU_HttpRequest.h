// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_RequestBase.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNU_HttpRequest.generated.h"

class UJWNU_GIS_HttpClientHelper;
class UJWNU_HttpRequestJob;
class UWorld;

/** 직접 URL을 사용하는 HTTP 요청의 종료 오류 분류다. */
UENUM(BlueprintType)
enum class EJWNU_HttpRequestError : uint8 { None, RequestFailed, Cancelled };

/** HTTP 상태 코드와 서버가 반환한 원문 응답이다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_HttpResult
{
    GENERATED_BODY()
    /** HTTP 정수 상태 코드 필드. 응답이 없으면 0이다. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|HTTP") int32 StatusCode = 0;
    /** 오류 응답을 포함해 정규화하지 않은 응답 본문 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|HTTP") FString ResponseBody;
};

/** HTTP 실패·취소와 원문 응답을 담는 진단 데이터다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_HttpError
{
    GENERATED_BODY()
    /** 요청 실패와 취소를 구분하는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|HTTP") EJWNU_HttpRequestError Code = EJWNU_HttpRequestError::None;
    /** HTTP 상태와 원문 오류 응답 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|HTTP") FJWNU_HttpResult Response;
    /** 전송 시작 전 입력 오류 등의 진단 메시지 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|HTTP") FString Message;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_HttpCompletedBP, const FJWNU_HttpResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_HttpFailedBP, const FJWNU_HttpError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_HttpRetryBP, int32, AttemptNumber);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_HttpCompletedNative, const FJWNU_HttpResult&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_HttpFailedNative, const FJWNU_HttpError&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_HttpRetryNative, int32);

/** 생성·바인딩·Start 순서로 사용하는 일회용 직접 URL HTTP 요청이다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITY_API UJWNU_HttpRequest : public UJWNU_RequestBase
{
    GENERATED_BODY()
public:
    /** 전송 없이 요청을 생성하는 함수. 변수에 저장하고 이벤트를 바인딩한 뒤 Start한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|HTTP", meta=(WorldContext="WorldContextObject", DisplayName="Create HTTP Request"))
    static UJWNU_HttpRequest* CreateHttpRequest(const UObject* WorldContextObject);
    /** 절대 HTTP(S) URL로 JSON 요청을 시작하는 함수. API Key는 선택적 Bearer 값이며 JWT 자동 갱신은 하지 않는다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|HTTP", meta=(AutoCreateRefTerm="QueryParams"))
    bool Start(EJWNU_HttpMethod Method, const FString& URL, const FString& ContentBody, const TMap<FString, FString>& QueryParams, UPARAM(DisplayName="API Key") const FString& ApiKey = TEXT(""));
    /** 마지막 성공 응답을 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|HTTP") FJWNU_HttpResult GetResult() const { return Result; }
    /** 마지막 오류를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|HTTP") FJWNU_HttpError GetError() const { return Error; }
    /** HTTP 2xx 원문 응답을 전달하는 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|HTTP") FJWNU_HttpCompletedBP OnCompleted;
    /** HTTP·입력·전송 실패 또는 취소를 전달하는 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|HTTP") FJWNU_HttpFailedBP OnFailed;
    /** 재시도 번호를 전달하는 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|HTTP") FJWNU_HttpRetryBP OnRetry;
    FJWNU_HttpCompletedNative OnCompletedNative;
    FJWNU_HttpFailedNative OnFailedNative;
    FJWNU_HttpRetryNative OnRetryNative;
private:
    virtual void CancelRequest() override;
    friend class UJWNU_GIS_HttpClientHelper;
    void Receive(int32 Status, const FString& Body);
    void Retry(int32 AttemptNumber);
    void Finish();
    /** 활성 전송 Job을 보관하는 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_HttpRequestJob> Job;
    /** 마지막 성공 응답 필드. */
    UPROPERTY(Transient) FJWNU_HttpResult Result;
    /** 마지막 실패 진단 필드. */
    UPROPERTY(Transient) FJWNU_HttpError Error;
    TWeakObjectPtr<UJWNU_GIS_HttpClientHelper> OwnerClient;
    TWeakObjectPtr<UWorld> OwnerWorld;
};
