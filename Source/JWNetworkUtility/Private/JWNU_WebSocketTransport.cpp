// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_WebSocketTransport.h"
#include "JWNU_WebSocketReceiveQueue.h"
#include "HAL/RunnableThread.h"
#include "Misc/ScopeLock.h"
#include "Modules/ModuleManager.h"
#include "Ssl.h"
#include "GenericPlatform/GenericPlatformHttp.h"

FJWNU_WebSocketTransport::FJWNU_WebSocketTransport(const FString& URL, const FJWNU_WebSocketOptions& Options,
	TSharedRef<FJWNU_WebSocketReceiveQueue, ESPMode::ThreadSafe> InReceive)
	: Address(URL), Settings(Options), Receive(InReceive)
{
}

bool FJWNU_WebSocketTransport::Start()
{
	check(IsInGameThread());
	auto& Manager = FModuleManager::LoadModuleChecked<FSslModule>("SSL").GetSslManager();
	// Editor DLL에서는 엔진 SSL manager의 초기화 함수가 의도적으로 false다.
	// 이 모듈에 링크된 OpenSSL로 context를 만들고 엔진의 인증서 저장소를 사용한다.
	if (Manager.InitializeSsl()) { SslManager = &Manager; }
	SslContext = SSL_CTX_new(TLS_client_method());
	if (!SslContext) { return false; }
	SSL_CTX_set_min_proto_version(SslContext, TLS1_2_VERSION);
	SSL_CTX_set_options(SslContext, SSL_OP_NO_COMPRESSION);
	SSL_CTX_set_verify(SslContext, SSL_VERIFY_PEER, nullptr);
	FSslModule::Get().GetCertificateManager().AddCertificatesToSslContext(SslContext);
	Protocols[0].name = "jwnu";
	Protocols[0].callback = &FJWNU_WebSocketTransport::Callback;
	Protocols[0].rx_buffer_size = 16384;
	Thread = FRunnableThread::Create(this, TEXT("JWNU_WebSocket"), 128 * 1024, TPri_BelowNormal);
	return Thread != nullptr;
}

FJWNU_WebSocketTransport::~FJWNU_WebSocketTransport()
{
	Stop();
	if (Thread) { Thread->WaitForCompletion(); delete Thread; }
	if (SslContext) { SSL_CTX_free(SslContext); }
	if (SslManager) { SslManager->ShutdownSsl(); }
}

void FJWNU_WebSocketTransport::Wake()
{
	FScopeLock Lock(&ContextMutex);
	if (Context) { lws_cancel_service(Context); }
}

void FJWNU_WebSocketTransport::Stop()
{
	bStop.store(true);
	Wake();
}

bool FJWNU_WebSocketTransport::Send(const void* Data, int32 Size, bool bBinary)
{
	{
		FScopeLock Lock(&SendMutex);
		const int64 Cost = static_cast<int64>(Size) + LWS_PRE + sizeof(FSend);
		if (!bConnected.load() || bStop.load() || bCloseRequested || Cost > Settings.MaxSendQueuedBytes - PendingSendBytes) { return false; }
		FSend Item;
		Item.bBinary = bBinary;
		Item.Bytes.AddZeroed(LWS_PRE + Size);
		if (Size) { FMemory::Memcpy(Item.Bytes.GetData() + LWS_PRE, Data, Size); }
		PendingSendBytes += Cost;
		Sends.Enqueue(MoveTemp(Item));
	}
	Wake();
	return true;
}

void FJWNU_WebSocketTransport::Close(int32 Code, const FString& Reason)
{
	{
		FScopeLock Lock(&SendMutex);
		bCloseRequested = true; CloseCode = Code;
		const FTCHARToUTF8 Utf8(*Reason, Reason.Len());
		CloseReason.Reset();
		if (Utf8.Length()) { CloseReason.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()); }
	}
	Wake();
}

void FJWNU_WebSocketTransport::Error(const FString& Message)
{
	if (bTerminal || bStop.load()) { return; }
	bTerminal = true; bConnected.store(false);
	FJWNU_WebSocketReceiveQueue::FItem Item;
	Item.Kind = FJWNU_WebSocketReceiveQueue::FItem::EKind::Error;
	Item.Text = Message;
	Receive->Push(MoveTemp(Item));
}

void FJWNU_WebSocketTransport::Closed()
{
	if (bTerminal || bStop.load()) { return; }
	bTerminal = true; bConnected.store(false);
	FJWNU_WebSocketReceiveQueue::FItem Item;
	Item.Kind = FJWNU_WebSocketReceiveQueue::FItem::EKind::Closed;
	if (bPeerClose) { Item.Close = PeerClose; }
	else if (bCloseSent)
	{
		FScopeLock Lock(&SendMutex);
		Item.Close.Code = CloseCode;
		const FUTF8ToTCHAR Utf8(CloseReason.IsEmpty() ? "" : reinterpret_cast<const ANSICHAR*>(CloseReason.GetData()), CloseReason.Num());
		Item.Close.Reason = FString(Utf8.Length(), Utf8.Get());
		Item.Close.bWasClean = true;
	}
	else { Item.Close.Code = 1006; Item.Close.Reason = TEXT("WebSocket transport disconnected"); }
	Receive->Push(MoveTemp(Item));
}

uint32 FJWNU_WebSocketTransport::Run()
{
	lws_context_creation_info Info = {};
	Info.port = CONTEXT_PORT_NO_LISTEN;
	Info.protocols = Protocols; Info.uid = -1; Info.gid = -1;
	Info.user = this;
	Info.provided_client_ssl_ctx = SslContext;
	Info.options = LWS_SERVER_OPTION_DISABLE_OS_CA_CERTS | LWS_SERVER_OPTION_VALIDATE_UTF8;
	Info.max_http_header_data2 = 32768;
	auto* LocalContext = lws_create_context(&Info);
	if (!LocalContext) { Error(TEXT("Failed to initialize WebSocket transport")); return 0; }
	{
		FScopeLock Lock(&ContextMutex); Context = LocalContext;
	}
	FTCHARToUTF8 URL(*Address);
	const char* Scheme = nullptr; const char* Host = nullptr; const char* Path = nullptr; int Port = 0;
	if (lws_parse_uri(const_cast<ANSICHAR*>(URL.Get()), &Scheme, &Host, &Port, &Path)) { Error(TEXT("Invalid WebSocket URL")); }
	else if (!bStop.load())
	{
		const FString RequestPath = FString(TEXT("/")) + UTF8_TO_TCHAR(Path);
		FTCHARToUTF8 PathUtf8(*RequestPath);
		FTCHARToUTF8 ProtocolUtf8(*FString::Join(Settings.Protocols, TEXT(",")));
		lws_client_connect_info ConnectInfo = {};
		ConnectInfo.context = LocalContext; ConnectInfo.address = Host; ConnectInfo.port = Port;
		ConnectInfo.path = PathUtf8.Get(); ConnectInfo.host = Host;
		ConnectInfo.ssl_connection = Address.StartsWith(TEXT("wss://")) ? LCCSCF_USE_SSL : 0;
		ConnectInfo.protocol = Settings.Protocols.IsEmpty() ? nullptr : ProtocolUtf8.Get();
		ConnectInfo.local_protocol_name = "jwnu";
		ConnectInfo.userdata = this; ConnectInfo.pwsi = &Connection;
		if (!lws_client_connect_via_info(&ConnectInfo)) { Error(TEXT("WebSocket connection could not start")); }
	}
	while (!bStop.load() && !bTerminal)
	{
		if (Connection && bConnected.load())
		{
			FScopeLock Lock(&SendMutex);
			if (bCloseRequested || !Sends.IsEmpty()) { lws_callback_on_writable(Connection); }
		}
		if (lws_service(LocalContext, 0) < 0) { Error(TEXT("WebSocket service failed")); }
	}
	{
		FScopeLock Lock(&ContextMutex); Context = nullptr;
	}
	lws_context_destroy(LocalContext);
	Connection = nullptr;
	return 0;
}

int FJWNU_WebSocketTransport::Callback(lws* Socket, lws_callback_reasons Reason, void* User, void* Data, size_t Size)
{
	auto* Self = static_cast<FJWNU_WebSocketTransport*>(lws_context_user(lws_get_context(Socket)));
	if (Reason == LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION)
	{
		// 인증서 체인·호스트 검증 실패를 성공으로 덮어쓰지 않는다. UE 인증서 정책도 적용한다.
		return Self && !Self->bStop.load() && Size == 1
			&& FSslModule::Get().GetCertificateManager().VerifySslCertificates(static_cast<X509_STORE_CTX*>(User),
				FGenericPlatformHttp::GetUrlDomain(Self->Address)) ? 0 : 1;
	}
	return Self ? Self->ReceiveCallback(Socket, Reason, Data, Size) : 0;
}

int FJWNU_WebSocketTransport::ReceiveCallback(lws* Socket, lws_callback_reasons Reason, void* Data, size_t Size)
{
	using FItem = FJWNU_WebSocketReceiveQueue::FItem;
	if (bStop.load()) { return 0; }
	switch (Reason)
	{
	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
	{
		auto** Position = static_cast<unsigned char**>(Data);
		unsigned char* End = *Position + Size;
		for (const auto& Header : Settings.Headers)
		{
			FTCHARToUTF8 Key(*(Header.Key + TEXT(":"))); FTCHARToUTF8 Value(*Header.Value);
			if (lws_add_http_header_by_name(Socket, reinterpret_cast<const unsigned char*>(Key.Get()),
				reinterpret_cast<const unsigned char*>(Value.Get()), Value.Length(), Position, End)) { Error(TEXT("WebSocket headers exceed transport limit")); return -1; }
		}
		break;
	}
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
	{
		bConnected.store(true);
		FItem Item; Item.Kind = FItem::EKind::Connected; Receive->Push(MoveTemp(Item));
		break;
	}
	case LWS_CALLBACK_CLIENT_RECEIVE:
	{
		const bool bLast = lws_is_final_fragment(Socket) && lws_remaining_packet_payload(Socket) == 0;
		if (lws_frame_is_binary(Socket)) { Receive->PushBinary(Data, Size, bLast); }
		else { Receive->PushTextFragment(Data, Size, bLast); }
		break;
	}
	case LWS_CALLBACK_CLIENT_WRITEABLE:
	{
		FSend Item;
		{
			FScopeLock Lock(&SendMutex);
			if (bCloseRequested)
			{
				bCloseSent = true;
				lws_close_reason(Socket, static_cast<lws_close_status>(CloseCode), CloseReason.GetData(), CloseReason.Num());
				return -1;
			}
			if (!Sends.Dequeue(Item)) { break; }
		}
		const int32 PayloadSize = Item.Bytes.Num() - LWS_PRE;
		const int Written = lws_write(Socket, Item.Bytes.GetData() + LWS_PRE, PayloadSize, Item.bBinary ? LWS_WRITE_BINARY : LWS_WRITE_TEXT);
		{
			FScopeLock Lock(&SendMutex); PendingSendBytes -= Item.Bytes.Num() + sizeof(FSend);
			if (!Sends.IsEmpty()) { lws_callback_on_writable(Socket); }
		}
		if (Written < PayloadSize) { Error(TEXT("WebSocket send failed")); return -1; }
		break;
	}
	case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
	{
		bPeerClose = true; PeerClose.bWasClean = true;
		const uint8* Bytes = static_cast<const uint8*>(Data);
		PeerClose.Code = Size >= 2 ? (static_cast<int32>(Bytes[0]) << 8) | Bytes[1] : 1005;
		if (Size > 2)
		{
			FUTF8ToTCHAR Utf8(reinterpret_cast<const ANSICHAR*>(Bytes + 2), Size - 2);
			PeerClose.Reason = FString(Utf8.Length(), Utf8.Get());
		}
		return 0; // libwebsockets가 Close 응답을 보낸다.
	}
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
	{
		FUTF8ToTCHAR Utf8(Data ? static_cast<const ANSICHAR*>(Data) : "", Data ? Size : 0);
		Error(FString::Printf(TEXT("WebSocket connection error: %s"), *FString(Utf8.Length(), Utf8.Get())));
		Connection = nullptr;
		break;
	}
	case LWS_CALLBACK_CLIENT_CLOSED:
	case LWS_CALLBACK_CLOSED:
	case LWS_CALLBACK_WSI_DESTROY:
		if (Socket == Connection) { Closed(); Connection = nullptr; }
		break;
	default: break;
	}
	return 0;
}
