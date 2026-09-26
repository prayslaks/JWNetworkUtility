// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNU_SseTypes.generated.h"

/** SSE 프레임에서 조립한 이벤트와 현재 재연결 힌트를 담는다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_SseEvent
{
	GENERATED_BODY()
	/** 기본값이 message인 이벤트 종류 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") FString EventType;
	/** 여러 data 줄을 개행으로 합친 원문 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") FString Data;
	/** 마지막으로 수신한 이벤트 ID 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") FString Id;
	/** 서버의 재연결 권고 시간이며 -1은 미지정인 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") int64 RetryMilliseconds = -1;
};

/** HTTP 상태와 구분하는 스트림 종료 원인이다. */
UENUM(BlueprintType)
enum class EJWNU_SseError : uint8
{
	None, InvalidRequest, Http, Network, Timeout, InvalidContentType, BufferLimit, Authentication, Cancelled
};

/** 연결 및 종료 알림의 HTTP 메타데이터와 오류를 담는다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_SseResponse
{
	GENERATED_BODY()
	/** 원래 HTTP 상태 코드 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") int32 StatusCode = 0;
	/** 소문자 헤더 이름을 키로 사용하는 응답 헤더 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") TMap<FString, FString> Headers;
	/** 스트림 오류 종류 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") EJWNU_SseError Error = EJWNU_SseError::None;
	/** 진단 메시지 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") FString Message;
	/** 비SSE 오류 응답의 크기가 제한된 원문 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") FString ErrorBody;
	/** 오류 본문이 상한에 의해 잘렸는지 나타내는 필드. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|SSE") bool bErrorBodyTruncated = false;
};

/** SSE 요청별 헤더와 버퍼 및 대기 제한을 정의한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_SseOptions
{
	GENERATED_BODY()
	/** Authorization을 포함해 기본 헤더를 덮어쓸 수 있는 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|SSE") TMap<FString, FString> Headers;
	/** 응답을 열기까지 기다리는 최대 시간 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|SSE", meta=(ClampMin="0")) float OpenTimeoutSeconds = 30.f;
	/** 수신 바이트가 없는 구간의 최대 시간이며 0은 무제한인 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|SSE", meta=(ClampMin="0")) float IdleTimeoutSeconds = 30.f;
	/** 전체 요청 시간 상한이며 0은 무제한인 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|SSE", meta=(ClampMin="0")) float TotalTimeoutSeconds = 0.f;
	/** 한 이벤트와 한 줄의 최대 바이트 크기 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|SSE", meta=(ClampMin="1")) int32 MaxEventBytes = 1048576;
	/** 게임 스레드가 처리하기 전 수신 큐의 최대 바이트 크기 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|SSE", meta=(ClampMin="1")) int32 MaxQueuedBytes = 4194304;
	/** 오류 응답 본문의 최대 보관 바이트 크기 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|SSE", meta=(ClampMin="1")) int32 MaxErrorBodyBytes = 65536;
};

DECLARE_DELEGATE_OneParam(FJWNU_OnSseEvent, const FJWNU_SseEvent&);
DECLARE_DELEGATE_OneParam(FJWNU_OnSseResponse, const FJWNU_SseResponse&);
DECLARE_DELEGATE(FJWNU_OnSseCancelled);

/** 요청 객체와 내부 전송 Job이 공유하는 게임 스레드 콜백 묶음이다. */
struct JWNETWORKUTILITY_API FJWNU_SseCallbacks
{
	FJWNU_OnSseResponse OnOpened;
	FJWNU_OnSseEvent OnEvent;
	FJWNU_OnSseResponse OnCompleted;
	FJWNU_OnSseResponse OnError;
	FJWNU_OnSseCancelled OnCancelled;
};
