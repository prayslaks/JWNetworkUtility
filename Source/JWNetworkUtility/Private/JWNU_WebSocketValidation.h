// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "JWNU_WebSocketTransport.h"
#include "Containers/StringConv.h"

namespace JWNU::WebSocket
{
inline bool IsToken(const FString& Value)
{
	if (Value.IsEmpty()) { return false; }
	for (TCHAR C : Value)
	{
		if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9')
			|| FString(TEXT("!#$%&'*+-.^_`|~")).Contains(FString::Chr(C)))) { return false; }
	}
	return true;
}
inline bool IsURL(const FString& URL)
{
	const int32 Start = URL.StartsWith(TEXT("ws://")) ? 5 : URL.StartsWith(TEXT("wss://")) ? 6 : 0;
	if (!Start) { return false; }
	for (TCHAR C : URL) { if (C <= 32 || C == '#' || C == '\\') { return false; } }
	FString Authority = URL.Mid(Start);
	int32 End;
	if (Authority.FindChar('/', End)) { Authority.LeftInline(End); }
	if (Authority.FindChar('?', End)) { Authority.LeftInline(End); }
	if (Authority.IsEmpty() || Authority.Contains(TEXT("@"))) { return false; }
	FString Port;
	if (Authority.StartsWith(TEXT("[")))
	{
		if (!Authority.FindChar(']', End) || End <= 1) { return false; }
		const FString Suffix = Authority.Mid(End + 1);
		if (!Suffix.IsEmpty() && !Suffix.StartsWith(TEXT(":"))) { return false; }
		if (Suffix.IsEmpty()) { return true; }
		Port = Suffix.Mid(1);
	}
	else if (Authority.FindChar(':', End))
	{
		if (End == 0) { return false; }
		Port = Authority.Mid(End + 1);
	}
	else { return true; }
	if (Port.IsEmpty() || Port.Len() > 5) { return false; }
	for (TCHAR C : Port) { if (C < '0' || C > '9') { return false; } }
	return FCString::Atoi(*Port) <= 65535 && FCString::Atoi(*Port) > 0;
}
inline bool ValidSettings(const FJWNU_WebSocketOptions& Options)
{
	if (Options.MaxMessageBytes < 1 || Options.MaxMessageBytes > MAX_int32 - LWS_PRE || Options.MaxQueuedBytes < 1 || Options.MaxSendQueuedBytes < 1
		|| !FMath::IsFinite(Options.ConnectTimeoutSeconds) || Options.ConnectTimeoutSeconds < 0
		|| !FMath::IsFinite(Options.CloseTimeoutSeconds) || Options.CloseTimeoutSeconds <= 0) { return false; }
	for (const auto& Pair : Options.Headers)
	{
		if (!IsToken(Pair.Key)) { return false; }
		for (TCHAR C : Pair.Value) { if (C == 0 || C == '\r' || C == '\n') { return false; } }
	}
	for (const FString& Protocol : Options.Protocols) { if (!IsToken(Protocol)) { return false; } }
	return true;
}
inline bool ValidClose(int32 Code, const FString& Reason)
{
	const bool bCode = (Code >= 3000 && Code <= 4999)
		|| (Code >= 1000 && Code <= 1014 && Code != 1004 && Code != 1005 && Code != 1006);
	const FTCHARToUTF8 Utf8(*Reason, Reason.Len());
	for (TCHAR C : Reason) { if (C == 0) { return false; } }
	return bCode && Utf8.Length() <= 123;
}
}

