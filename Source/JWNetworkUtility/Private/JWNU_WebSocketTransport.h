// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Containers/Queue.h"
#include "JWNU_WebSocketTypes.h"
#include <atomic>

THIRD_PARTY_INCLUDES_START
#pragma push_macro("UI")
#undef UI
#define UI JWNU_OpenSslUI
#include "libwebsockets.h"
#pragma pop_macro("UI")
THIRD_PARTY_INCLUDES_END

class FRunnableThread;
class ISslManager;
struct ssl_ctx_st;
struct FJWNU_WebSocketReceiveQueue;

/** 엔진에 포함된 libwebsockets를 사용하는 전용 I/O 드라이버. UObject 및 BP를 접근하지 않는다. */
class FJWNU_WebSocketTransport final : public FRunnable
{
public:
	FJWNU_WebSocketTransport(const FString& URL, const FJWNU_WebSocketOptions& Options,
		TSharedRef<FJWNU_WebSocketReceiveQueue, ESPMode::ThreadSafe> InReceive);
	virtual ~FJWNU_WebSocketTransport() override;
	bool Start();
	bool Send(const void* Data, int32 Size, bool bBinary);
	void Close(int32 Code, const FString& Reason);
	virtual void Stop() override;
	virtual uint32 Run() override;
private:
	struct FSend { TArray<uint8> Bytes; bool bBinary = false; };
	static int Callback(lws* Socket, lws_callback_reasons Reason, void* User, void* Data, size_t Size);
	int ReceiveCallback(lws* Socket, lws_callback_reasons Reason, void* Data, size_t Size);
	void Wake();
	void Error(const FString& Message);
	void Closed();
	FString Address;
	FJWNU_WebSocketOptions Settings;
	TSharedRef<FJWNU_WebSocketReceiveQueue, ESPMode::ThreadSafe> Receive;
	FRunnableThread* Thread = nullptr;
	ISslManager* SslManager = nullptr;
	ssl_ctx_st* SslContext = nullptr;
	FCriticalSection ContextMutex;
	lws_context* Context = nullptr;
	lws* Connection = nullptr; // I/O 스레드 전용
	lws_protocols Protocols[2] = {};
	FCriticalSection SendMutex;
	TQueue<FSend> Sends;
	int64 PendingSendBytes = 0;
	bool bCloseRequested = false;
	int32 CloseCode = 1000;
	TArray<uint8> CloseReason;
	std::atomic<bool> bStop{false};
	std::atomic<bool> bConnected{false};
	bool bTerminal = false; // 아래 필드는 I/O 스레드 전용
	bool bCloseSent = false;
	bool bPeerClose = false;
	FJWNU_WebSocketCloseInfo PeerClose;
};
