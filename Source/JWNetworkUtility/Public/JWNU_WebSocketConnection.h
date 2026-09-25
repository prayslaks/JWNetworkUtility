// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_WebSocketTypes.h"
#include "JWNU_WebSocketConnection.generated.h"

class FJWNU_WebSocketTransport;
class UWorld;
class UJWNU_GIS_WebSocketClient;
struct FJWNU_WebSocketReceiveQueue;

/** HTTP Job과 독립적인 WebSocket 연결 핸들이다. 모든 공개 호출과 이벤트는 게임 스레드에서 사용한다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITY_API UJWNU_WebSocketConnection : public UObject
{
	GENERATED_BODY()
public:
	/** 연결을 다음 Tick에 시작하는 함수. 종료된 핸들로 새 연결도 시작할 수 있다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|WebSocket", meta=(AutoCreateRefTerm="Options"))
	bool Connect(const FString& URL, const FJWNU_WebSocketOptions& Options);
	/** 연결된 소켓에 텍스트 전송을 접수하는 함수. true는 상대 수신 확인이 아니다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|WebSocket")
	bool SendText(const FString& Message);
	/** 연결된 소켓에 바이너리 전송을 접수하는 함수. 연결 전·종료 중·상한 초과는 false다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|WebSocket")
	bool SendBinary(const TArray<uint8>& Data);
	/** 정상 종료 협상을 시작하는 함수. 연결 중에도 취소할 수 있다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|WebSocket")
	bool Close(int32 Code = 1000, const FString& Reason = TEXT(""));
	UFUNCTION(BlueprintPure, Category="JWNU|WebSocket")
	EJWNU_WebSocketState GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category="JWNU|WebSocket")
	bool IsConnected() const { return State == EJWNU_WebSocketState::Connected; }
	UFUNCTION(BlueprintPure, Category="JWNU|WebSocket")
	bool IsActive() const;

	UPROPERTY(BlueprintAssignable, Category="JWNU|WebSocket") FJWNU_WebSocketConnectedBP OnConnected;
	UPROPERTY(BlueprintAssignable, Category="JWNU|WebSocket") FJWNU_WebSocketTextBP OnTextMessage;
	UPROPERTY(BlueprintAssignable, Category="JWNU|WebSocket") FJWNU_WebSocketBinaryBP OnBinaryMessage;
	UPROPERTY(BlueprintAssignable, Category="JWNU|WebSocket") FJWNU_WebSocketErrorBP OnError;
	UPROPERTY(BlueprintAssignable, Category="JWNU|WebSocket") FJWNU_WebSocketClosedBP OnClosed;

	FSimpleMulticastDelegate OnConnectedNative;
	FJWNU_WebSocketTextNative OnTextMessageNative;
	FJWNU_WebSocketBinaryNative OnBinaryMessageNative;
	FJWNU_WebSocketErrorNative OnErrorNative;
	FJWNU_WebSocketClosedNative OnClosedNative;

	virtual void BeginDestroy() override;
private:
	friend class UJWNU_GIS_WebSocketClient;
	void BeginConnect();
	void Pump();
	void Shutdown();
	void ReleaseTransport();
	void Finish(const FJWNU_WebSocketCloseInfo& Info, EJWNU_WebSocketError Error = EJWNU_WebSocketError::None);
	void Fail(EJWNU_WebSocketError Error, const FString& Message);
	TWeakObjectPtr<UWorld> OwnerWorld;
	TWeakObjectPtr<UJWNU_GIS_WebSocketClient> OwnerClient;
	TSharedPtr<FJWNU_WebSocketTransport> Socket;
	TSharedPtr<FJWNU_WebSocketReceiveQueue, ESPMode::ThreadSafe> Queue;
	FJWNU_WebSocketOptions Settings;
	FJWNU_WebSocketCloseInfo LocalClose;
	FString Address;
	EJWNU_WebSocketState State = EJWNU_WebSocketState::Idle;
	double StartedSeconds = 0;
	double ClosingSeconds = 0;
	bool bPendingConnect = false;
	bool bDispatchingTerminal = false;
};
