// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNU_SseTypes.h"
#include "UObject/Object.h"
#include "JWNU_SseTestReceiver.generated.h"

class UJWNU_SseRequest;
class UJWNU_SseApiRequest;

/** Code와 Message 필드 없이 템플릿 변환을 검증하는 테스트 데이터다. */
USTRUCT(BlueprintType)
struct FJWNU_SseTestPayload
{
	GENERATED_BODY()
	/** 모의 서버가 보낸 이벤트 순서를 검증할 인덱스 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SSE Test") int32 Index = -1;
	/** UTF-8 한글 이벤트 전달을 확인할 텍스트 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SSE Test") FString Text;
	/** 서버가 돌려준 요청 입력의 에코 문자열 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SSE Test") FString Echo;
	/** 서버가 관찰한 요청 헤더 값을 확인하는 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SSE Test") FString Header;
};

/** BP 동적 델리게이트와 실제 JSON 변환 그래프의 수신 객체다. */
UCLASS(Blueprintable)
class UJWNU_SseTestReceiver : public UObject
{
	GENERATED_BODY()
public:
	/** Options·QueryParams 미연결 직접 URL Start를 실행하는 함수. */
	UFUNCTION(BlueprintImplementableEvent, Category="SSE Test") void StartDirectDefaults(UJWNU_SseRequest* Target, const FString& Address);
	/** Options·QueryParams 미연결 서비스 Start를 실행하는 함수. */
	UFUNCTION(BlueprintImplementableEvent, Category="SSE Test") void StartServiceDefaults(UJWNU_SseApiRequest* Target, const FString& Address);
	/** BP 변환을 거쳐 마지막으로 수신한 이벤트 데이터를 보관하는 필드. */
	UPROPERTY(BlueprintReadOnly, Category="SSE Test") FJWNU_SseTestPayload LastPayload;
	int32 EventCount = 0;
	int32 OpenCount = 0;
	int32 TerminalCount = 0;
	int32 ParseErrorCount = 0;
	double FirstEventSeconds = 0;
	double CompletedSeconds = 0;
	FJWNU_SseResponse LastResponse;
	bool bCancelled = false;
	/** 원시 SSE 이벤트를 BP 그래프 또는 기본 JSON 변환 구현으로 전달하는 함수. */
	UFUNCTION(BlueprintNativeEvent, Category="SSE Test") void ReceiveEvent(const FJWNU_SseEvent& Event);
	virtual void ReceiveEvent_Implementation(const FJWNU_SseEvent& Event);
	/** 변환된 이벤트 데이터와 수신 횟수·첫 수신 시각을 기록하는 함수. */
	UFUNCTION(BlueprintCallable, Category="SSE Test") void RecordParsed(const FJWNU_SseTestPayload& Payload);
	/** 스트림 열림 횟수와 응답 정보를 기록하는 함수. */
	UFUNCTION() void ReceiveOpened(const FJWNU_SseResponse& Response);
	/** 정상 종료 횟수와 응답 정보·완료 시각을 기록하는 함수. */
	UFUNCTION() void ReceiveCompleted(const FJWNU_SseResponse& Response);
	/** 실패 횟수와 응답 정보를 기록하고 취소 오류 여부를 구분하는 함수. */
	UFUNCTION() void ReceiveError(const FJWNU_SseResponse& Response);
	/** 즉시 실행 노드의 취소 전용 콜백을 기록하는 함수. */
	UFUNCTION() void ReceiveCancelled();
};
