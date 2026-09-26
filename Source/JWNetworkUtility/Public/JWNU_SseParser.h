// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNU_SseTypes.h"

/** 임의 경계의 UTF-8 바이트를 SSE 이벤트로 조립하는 비UObject 파서다. */
class JWNETWORKUTILITY_API FJWNU_SseParser
{
public:
	/** 파서 상태와 이벤트 크기 상한을 초기화하는 함수. */
	void Reset(int32 InMaxEventBytes = 1048576);
	/** 수신 조각을 처리하며 완성한 이벤트를 출력하는 함수. */
	bool Feed(const uint8* Bytes, int32 Length, TArray<FJWNU_SseEvent>& OutEvents);
	/** EOF에서 종료 빈 줄이 없는 미완성 이벤트를 버리는 함수. */
	void Finish();
	/** 서버가 전달한 마지막 재연결 힌트를 반환하는 함수. */
	int64 GetRetryMilliseconds() const { return RetryMilliseconds; }
	/** 마지막 이벤트 ID를 반환하는 함수. */
	const FString& GetLastEventId() const { return LastId; }

private:
	bool ConsumeLine(TArray<FJWNU_SseEvent>& OutEvents);
	TArray<uint8> Line;
	FString Data;
	FString EventType;
	FString LastId;
	int64 RetryMilliseconds = -1;
	int32 MaxEventBytes = 1048576;
	int32 EventBytes = 0;
	bool bHasData = false;
	bool bSkipLF = false;
	bool bFirstLine = true;
};
