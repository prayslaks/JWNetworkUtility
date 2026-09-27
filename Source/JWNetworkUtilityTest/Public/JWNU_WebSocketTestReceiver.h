// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_WebSocketTypes.h"
#include "JWNU_WebSocketTestReceiver.generated.h"

/** 자동 테스트에서 실제 BP 이벤트 디스패치를 관찰하는 객체다. */
UCLASS(Blueprintable)
class UJWNU_WebSocketTestReceiver : public UObject
{
	GENERATED_BODY()
public:
	int32 ConnectedCount = 0, ClosedCount = 0, ErrorCount = 0, BinaryCount = 0, EmptyBinaryCount = 0;
	TArray<FString> Texts;
	TArray<uint8> LastBinary;
	FJWNU_WebSocketCloseInfo LastClose;
	FJWNU_WebSocketError LastError;
	/** 게임 스레드에서 연결 완료 이벤트 횟수를 기록하는 함수. */
	UFUNCTION() void ReceiveConnected() { check(IsInGameThread()); ++ConnectedCount; }
	/** 바이너리 수신 횟수와 데이터를 기록해 메시지 조립 결과를 검증하는 함수. */
	UFUNCTION() void ReceiveBinary(const TArray<uint8>& Bytes)
	{
		check(IsInGameThread()); ++BinaryCount;
		if (Bytes.IsEmpty()) { ++EmptyBinaryCount; }
		else { LastBinary = Bytes; }
	}
	/** 게임 스레드에서 종료 횟수와 마지막 종료 정보를 기록하는 함수. */
	UFUNCTION() void ReceiveClosed(const FJWNU_WebSocketCloseInfo& Info) { check(IsInGameThread()); ++ClosedCount; LastClose = Info; }
	/** 게임 스레드에서 오류 횟수와 마지막 오류를 기록하는 함수. */
	UFUNCTION() void ReceiveError(const FJWNU_WebSocketError& Error) { check(IsInGameThread()); ++ErrorCount; LastError = Error; }
	/** 텍스트 수신이 생성된 BP 그래프를 통과하는지 검증하는 함수. */
	UFUNCTION(BlueprintNativeEvent, Category="WebSocket Test") void ReceiveText(const FString& Message);
	virtual void ReceiveText_Implementation(const FString& Message) { RecordText(Message); }
	/** 게임 스레드에서 BP를 통과한 텍스트를 순서대로 저장하는 함수. */
	UFUNCTION(BlueprintCallable, Category="WebSocket Test") void RecordText(const FString& Message) { check(IsInGameThread()); Texts.Add(Message); }
};
