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
	UFUNCTION() void ReceiveConnected() { check(IsInGameThread()); ++ConnectedCount; }
	UFUNCTION() void ReceiveBinary(const TArray<uint8>& Bytes)
	{
		check(IsInGameThread()); ++BinaryCount;
		if (Bytes.IsEmpty()) { ++EmptyBinaryCount; }
		else { LastBinary = Bytes; }
	}
	UFUNCTION() void ReceiveClosed(const FJWNU_WebSocketCloseInfo& Info) { check(IsInGameThread()); ++ClosedCount; LastClose = Info; }
	UFUNCTION() void ReceiveError(const FJWNU_WebSocketError& Error) { check(IsInGameThread()); ++ErrorCount; LastError = Error; }
	UFUNCTION(BlueprintNativeEvent, Category="WebSocket Test") void ReceiveText(const FString& Message);
	virtual void ReceiveText_Implementation(const FString& Message) { RecordText(Message); }
	UFUNCTION(BlueprintCallable, Category="WebSocket Test") void RecordText(const FString& Message) { check(IsInGameThread()); Texts.Add(Message); }
};
