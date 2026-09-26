// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_WebSocketConnection.h"
#include "JWNU_GIS_WebSocketClient.h"
#include "JWNU_WebSocketReceiveQueue.h"
#include "JWNU_WebSocketValidation.h"
#include "Containers/StringConv.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

bool UJWNU_WebSocketConnection::IsActive() const
{
	return State == EJWNU_WebSocketState::Connecting || State == EJWNU_WebSocketState::Connected || State == EJWNU_WebSocketState::Closing;
}

bool UJWNU_WebSocketConnection::Connect(const FString& URL, const FJWNU_WebSocketOptions& Options)
{
	check(IsInGameThread());
	if (IsActive() || bDispatchingTerminal || !OwnerClient.IsValid() || !OwnerClient->Track(this)) { return false; }
	ReleaseTransport();
	Address = URL; Settings = Options; LocalClose = FJWNU_WebSocketCloseInfo();
	State = EJWNU_WebSocketState::Connecting;
	StartedSeconds = FPlatformTime::Seconds();
	bPendingConnect = true;
	return true;
}

void UJWNU_WebSocketConnection::BeginConnect()
{
	bPendingConnect = false;
	if (!JWNU::WebSocket::IsURL(Address) || !JWNU::WebSocket::ValidSettings(Settings))
	{
		Fail(EJWNU_WebSocketError::InvalidConfiguration, TEXT("Expected an absolute ws:// or wss:// URL, valid headers/protocols and positive limits"));
		return;
	}
	Queue = MakeShared<FJWNU_WebSocketReceiveQueue, ESPMode::ThreadSafe>(Settings);
	Socket = MakeShared<FJWNU_WebSocketTransport>(Address, Settings, Queue.ToSharedRef());
	if (!Socket->Start()) { Fail(EJWNU_WebSocketError::Connection, TEXT("Failed to start WebSocket I/O worker")); }
}

bool UJWNU_WebSocketConnection::SendText(const FString& Message)
{
	check(IsInGameThread());
	if (!IsConnected() || !Socket) { return false; }
	const FTCHARToUTF8 Utf8(*Message, Message.Len());
	if (Utf8.Length() > Settings.MaxMessageBytes) { return false; }
	// 길이가 명시된 전송으로 내장 NUL도 보존한다.
	static const uint8 Empty = 0;
	return Socket->Send(Utf8.Length() ? Utf8.Get() : reinterpret_cast<const ANSICHAR*>(&Empty), Utf8.Length(), false);
}

bool UJWNU_WebSocketConnection::SendBinary(const TArray<uint8>& Data)
{
	check(IsInGameThread());
	if (!IsConnected() || !Socket || Data.Num() > Settings.MaxMessageBytes) { return false; }
	static const uint8 Empty = 0;
	return Socket->Send(Data.IsEmpty() ? &Empty : Data.GetData(), Data.Num(), true);
}

bool UJWNU_WebSocketConnection::Close(int32 Code, const FString& Reason)
{
	check(IsInGameThread());
	if (!IsActive() || State == EJWNU_WebSocketState::Closing || !JWNU::WebSocket::ValidClose(Code, Reason)) { return false; }
	LocalClose.Code = Code; LocalClose.Reason = Reason; LocalClose.bWasLocal = true;
	State = EJWNU_WebSocketState::Closing; ClosingSeconds = FPlatformTime::Seconds();
	if (Socket)
	{
		Socket->Close(Code, Reason);
	}
	return true;
}

void UJWNU_WebSocketConnection::Pump()
{
	check(IsInGameThread());
	const TStrongObjectPtr<UJWNU_WebSocketConnection> KeepAlive(this);
	if (!IsActive()) { return; }
	if (bPendingConnect)
	{
		if (State == EJWNU_WebSocketState::Closing) { Finish(LocalClose); return; }
		BeginConnect();
	}
	if (!IsActive()) { return; }
	TArray<FJWNU_WebSocketReceiveQueue::FItem> Items;
	if (Queue && !Queue->Take(Items))
	{
		Fail(EJWNU_WebSocketError::BufferLimit, TEXT("WebSocket message or receive queue exceeded its limit"));
		return;
	}
	for (const auto& Item : Items)
	{
		if (!IsActive()) { return; }
		using EKind = FJWNU_WebSocketReceiveQueue::FItem::EKind;
		if (Item.Kind == EKind::Error)
		{
			if (State == EJWNU_WebSocketState::Closing) { Finish(LocalClose); return; }
			Fail(EJWNU_WebSocketError::Connection, Item.Text); return;
		}
		if (Item.Kind == EKind::Closed)
		{
			auto Info = Item.Close; Info.bWasLocal = LocalClose.bWasLocal;
			Finish(Info, Info.bWasClean ? EJWNU_WebSocketError::None : EJWNU_WebSocketError::Connection);
			return;
		}
		if (State == EJWNU_WebSocketState::Closing) { continue; }
		if (Item.Kind == EKind::Connected && State == EJWNU_WebSocketState::Connecting)
		{
			State = EJWNU_WebSocketState::Connected;
			OnConnectedNative.Broadcast();
			if (IsActive()) { OnConnected.Broadcast(); }
		}
		else if (State == EJWNU_WebSocketState::Connected && Item.Kind == EKind::Text)
		{
			OnTextMessageNative.Broadcast(Item.Text);
			if (IsConnected()) { OnTextMessage.Broadcast(Item.Text); }
		}
		else if (State == EJWNU_WebSocketState::Connected && Item.Kind == EKind::Binary)
		{
			OnBinaryMessageNative.Broadcast(Item.Bytes);
			if (IsConnected()) { OnBinaryMessage.Broadcast(Item.Bytes); }
		}
	}
	const double Now = FPlatformTime::Seconds();
	if (State == EJWNU_WebSocketState::Connecting && Settings.ConnectTimeoutSeconds > 0 && Now - StartedSeconds >= Settings.ConnectTimeoutSeconds)
	{
		Fail(EJWNU_WebSocketError::Timeout, TEXT("WebSocket connection timed out"));
	}
	else if (State == EJWNU_WebSocketState::Closing && Now - ClosingSeconds >= Settings.CloseTimeoutSeconds)
	{
		LocalClose.bWasClean = false;
		LocalClose.Code = 1006;
		LocalClose.Reason = TEXT("WebSocket close handshake timed out");
		Finish(LocalClose);
	}
}

void UJWNU_WebSocketConnection::Finish(const FJWNU_WebSocketCloseInfo& Info, EJWNU_WebSocketError Error)
{
	if (!IsActive()) { return; }
	const TStrongObjectPtr<UJWNU_WebSocketConnection> KeepAlive(this);
	State = Error == EJWNU_WebSocketError::None ? EJWNU_WebSocketState::Closed : EJWNU_WebSocketState::Failed;
	bPendingConnect = false;
	ReleaseTransport();
	// 종료 콜백 도중 새 세션이 시작되어 이전 Closed와 섞이는 것을 방지한다.
	bDispatchingTerminal = true;
	if (Error != EJWNU_WebSocketError::None)
	{
		FJWNU_WebSocketError Details; Details.Code = Error; Details.Message = Info.Reason;
		OnErrorNative.Broadcast(Details); OnError.Broadcast(Details);
	}
	OnClosedNative.Broadcast(Info); OnClosed.Broadcast(Info);
	bDispatchingTerminal = false;
}

void UJWNU_WebSocketConnection::Fail(EJWNU_WebSocketError Error, const FString& Message)
{
	auto Info = LocalClose;
	Info.Code = 1006; Info.Reason = Message; Info.bWasClean = false;
	Finish(Info, Error);
}

void UJWNU_WebSocketConnection::ReleaseTransport()
{
	if (Queue) { Queue->Stop(); Queue.Reset(); }
	if (Socket)
	{
		Socket->Stop();
		Socket.Reset();
	}
}

void UJWNU_WebSocketConnection::Shutdown()
{
	check(IsInGameThread());
	OwnerWorld.Reset();
	if (IsActive())
	{
		FJWNU_WebSocketCloseInfo Info; Info.Code = 1001; Info.Reason = TEXT("Owning world ended"); Info.bWasLocal = true;
		Finish(Info);
	}
}

void UJWNU_WebSocketConnection::BeginDestroy()
{
	ReleaseTransport();
	Super::BeginDestroy();
}
