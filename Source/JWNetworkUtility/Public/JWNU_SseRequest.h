// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "JWNU_RequestBase.h"
#include "JWNU_SseTypes.h"
#include "JWNU_SseRequest.generated.h"

class UJWNU_GIS_SseClient;
class UJWNU_HttpRequestJobHandle;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_SseEventBP, const FJWNU_SseEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_SseResponseBP, const FJWNU_SseResponse&, Response);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_SseEventNative, const FJWNU_SseEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_SseResponseNative, const FJWNU_SseResponse&);

/** 직접 URL·서비스 SSE 요청의 이벤트와 일회용 수명을 공유하는 기반 클래스다. */
UCLASS(Abstract, BlueprintType)
class JWNETWORKUTILITY_API UJWNU_SseRequestBase : public UJWNU_RequestBase
{
    GENERATED_BODY()
public:
    /** 정상 종료 응답을 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|SSE") FJWNU_SseResponse GetResult() const { return Result; }
    /** 실패 또는 취소 진단을 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|SSE") FJWNU_SseResponse GetError() const { return Error; }
    /** 스트림 개방 응답 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|SSE") FJWNU_SseResponseBP OnOpened;
    /** 완성된 SSE 이벤트 필드. Data는 기존 JSON 변환 노드로 처리한다. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|SSE") FJWNU_SseEventBP OnEvent;
    /** HTTP 정상 EOF 응답 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|SSE") FJWNU_SseResponseBP OnCompleted;
    /** 전송·인증·입력 실패 또는 취소 진단 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|SSE") FJWNU_SseResponseBP OnFailed;
    FJWNU_SseResponseNative OnOpenedNative;
    FJWNU_SseEventNative OnEventNative;
    FJWNU_SseResponseNative OnCompletedNative;
    FJWNU_SseResponseNative OnFailedNative;
protected:
    static UJWNU_SseRequestBase* Create(const UObject* WorldContextObject, TSubclassOf<UJWNU_SseRequestBase> Class);
    bool Begin();
    bool Attach(UJWNU_HttpRequestJobHandle* StartedHandle);
    FJWNU_SseCallbacks MakeCallbacks();
    TWeakObjectPtr<UJWNU_GIS_SseClient> OwnerClient;
    TWeakObjectPtr<UWorld> OwnerWorld;
private:
    virtual void CancelRequest() override;
    friend class UJWNU_GIS_SseClient;
    void ReceiveOpened(const FJWNU_SseResponse& Response);
    void ReceiveEvent(const FJWNU_SseEvent& Event);
    void ReceiveCompleted(const FJWNU_SseResponse& Response);
    void ReceiveFailed(const FJWNU_SseResponse& Response);
    void ReceiveCancelled();
    /** 인증 갱신에 따른 Job 교체를 추적하는 내부 핸들 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_HttpRequestJobHandle> Handle;
    /** 정상 종료 응답 필드. */
    UPROPERTY(Transient) FJWNU_SseResponse Result;
    /** 실패·취소 진단 필드. */
    UPROPERTY(Transient) FJWNU_SseResponse Error;
};

/** 전체 URL과 선택적 API Key를 사용하는 SSE 요청이다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITY_API UJWNU_SseRequest : public UJWNU_SseRequestBase
{
    GENERATED_BODY()
public:
    /** 전송 없이 직접 URL SSE 요청을 생성하는 함수. 이벤트 바인딩 후 Start한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SSE", meta=(WorldContext="WorldContextObject", DisplayName="Create SSE Request"))
    static UJWNU_SseRequest* CreateSseRequest(const UObject* WorldContextObject);
    /** HTTP(S) URL로 한 번 시작하는 함수. 미연결 QueryParams·Options는 기본값이다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SSE", meta=(AutoCreateRefTerm="QueryParams,Options"))
    bool Start(EJWNU_HttpMethod Method, const FString& URL, const FString& ContentBody,
        const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options, UPARAM(DisplayName="API Key") const FString& ApiKey = TEXT(""));
};

/** 등록된 Host·JWT와 개방 전 인증 갱신을 사용하는 SSE 요청이다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITY_API UJWNU_SseApiRequest : public UJWNU_SseRequestBase
{
    GENERATED_BODY()
public:
    /** 전송 없이 서비스 SSE 요청을 생성하는 함수. 이벤트 바인딩 후 Start한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SSE", meta=(WorldContext="WorldContextObject", DisplayName="Create SSE API Request"))
    static UJWNU_SseApiRequest* CreateSseApiRequest(const UObject* WorldContextObject);
    /** 서비스 Host·JWT로 한 번 시작하는 함수. 미연결 QueryParams·Options는 기본값이다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|SSE", meta=(AutoCreateRefTerm="QueryParams,Options"))
    bool Start(EJWNU_ServiceType ServiceType, EJWNU_HttpMethod Method, const FString& Endpoint, const FString& ContentBody,
        const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options, bool bRequiresAuth = true);
};
