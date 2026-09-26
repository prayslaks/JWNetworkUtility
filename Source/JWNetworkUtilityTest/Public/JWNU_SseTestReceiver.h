// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SSE Test") int32 Index = -1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SSE Test") FString Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SSE Test") FString Echo;
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
	UPROPERTY(BlueprintReadOnly, Category="SSE Test") FJWNU_SseTestPayload LastPayload;
	int32 EventCount = 0;
	int32 OpenCount = 0;
	int32 TerminalCount = 0;
	int32 ParseErrorCount = 0;
	double FirstEventSeconds = 0;
	double CompletedSeconds = 0;
	FJWNU_SseResponse LastResponse;
	bool bCancelled = false;
	UFUNCTION(BlueprintNativeEvent, Category="SSE Test") void ReceiveEvent(const FJWNU_SseEvent& Event);
	virtual void ReceiveEvent_Implementation(const FJWNU_SseEvent& Event);
	UFUNCTION(BlueprintCallable, Category="SSE Test") void RecordParsed(const FJWNU_SseTestPayload& Payload);
	UFUNCTION() void ReceiveOpened(const FJWNU_SseResponse& Response);
	UFUNCTION() void ReceiveCompleted(const FJWNU_SseResponse& Response);
	UFUNCTION() void ReceiveError(const FJWNU_SseResponse& Response);
	/** 즉시 실행 노드의 취소 전용 콜백을 기록하는 함수. */
	UFUNCTION() void ReceiveCancelled();
};
