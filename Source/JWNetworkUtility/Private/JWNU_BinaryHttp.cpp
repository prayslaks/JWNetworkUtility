// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
#include "JWNU_BinaryHttp.h"
#include "Misc/ScopeExit.h"
#if PLATFORM_WINDOWS
#pragma push_macro("DrawState")
#include "Windows/WindowsHWrapper.h"
// WinHTTP 상태 매크로가 다음 Unity 파일의 libwebsockets enum을 덮지 않도록 복원한다.
#pragma push_macro("HTTP_STATUS_CONTINUE")
#pragma push_macro("HTTP_STATUS_SWITCH_PROTOCOLS")
#pragma push_macro("HTTP_STATUS_OK")
#pragma push_macro("HTTP_STATUS_CREATED")
#pragma push_macro("HTTP_STATUS_ACCEPTED")
#pragma push_macro("HTTP_STATUS_PARTIAL")
#pragma push_macro("HTTP_STATUS_NO_CONTENT")
#pragma push_macro("HTTP_STATUS_RESET_CONTENT")
#pragma push_macro("HTTP_STATUS_PARTIAL_CONTENT")
#pragma push_macro("HTTP_STATUS_WEBDAV_MULTI_STATUS")
#pragma push_macro("HTTP_STATUS_AMBIGUOUS")
#pragma push_macro("HTTP_STATUS_MOVED")
#pragma push_macro("HTTP_STATUS_REDIRECT")
#pragma push_macro("HTTP_STATUS_REDIRECT_METHOD")
#pragma push_macro("HTTP_STATUS_NOT_MODIFIED")
#pragma push_macro("HTTP_STATUS_USE_PROXY")
#pragma push_macro("HTTP_STATUS_REDIRECT_KEEP_VERB")
#pragma push_macro("HTTP_STATUS_PERMANENT_REDIRECT")
#pragma push_macro("HTTP_STATUS_BAD_REQUEST")
#pragma push_macro("HTTP_STATUS_DENIED")
#pragma push_macro("HTTP_STATUS_PAYMENT_REQ")
#pragma push_macro("HTTP_STATUS_FORBIDDEN")
#pragma push_macro("HTTP_STATUS_NOT_FOUND")
#pragma push_macro("HTTP_STATUS_BAD_METHOD")
#pragma push_macro("HTTP_STATUS_NONE_ACCEPTABLE")
#pragma push_macro("HTTP_STATUS_PROXY_AUTH_REQ")
#pragma push_macro("HTTP_STATUS_REQUEST_TIMEOUT")
#pragma push_macro("HTTP_STATUS_CONFLICT")
#pragma push_macro("HTTP_STATUS_GONE")
#pragma push_macro("HTTP_STATUS_LENGTH_REQUIRED")
#pragma push_macro("HTTP_STATUS_PRECOND_FAILED")
#pragma push_macro("HTTP_STATUS_REQUEST_TOO_LARGE")
#pragma push_macro("HTTP_STATUS_URI_TOO_LONG")
#pragma push_macro("HTTP_STATUS_UNSUPPORTED_MEDIA")
#pragma push_macro("HTTP_STATUS_RETRY_WITH")
#pragma push_macro("HTTP_STATUS_SERVER_ERROR")
#pragma push_macro("HTTP_STATUS_NOT_SUPPORTED")
#pragma push_macro("HTTP_STATUS_BAD_GATEWAY")
#pragma push_macro("HTTP_STATUS_SERVICE_UNAVAIL")
#pragma push_macro("HTTP_STATUS_GATEWAY_TIMEOUT")
#pragma push_macro("HTTP_STATUS_VERSION_NOT_SUP")
#pragma push_macro("HTTP_STATUS_FIRST")
#pragma push_macro("HTTP_STATUS_LAST")
#include <winhttp.h>
#pragma pop_macro("HTTP_STATUS_LAST")
#pragma pop_macro("HTTP_STATUS_FIRST")
#pragma pop_macro("HTTP_STATUS_VERSION_NOT_SUP")
#pragma pop_macro("HTTP_STATUS_GATEWAY_TIMEOUT")
#pragma pop_macro("HTTP_STATUS_SERVICE_UNAVAIL")
#pragma pop_macro("HTTP_STATUS_BAD_GATEWAY")
#pragma pop_macro("HTTP_STATUS_NOT_SUPPORTED")
#pragma pop_macro("HTTP_STATUS_SERVER_ERROR")
#pragma pop_macro("HTTP_STATUS_RETRY_WITH")
#pragma pop_macro("HTTP_STATUS_UNSUPPORTED_MEDIA")
#pragma pop_macro("HTTP_STATUS_URI_TOO_LONG")
#pragma pop_macro("HTTP_STATUS_REQUEST_TOO_LARGE")
#pragma pop_macro("HTTP_STATUS_PRECOND_FAILED")
#pragma pop_macro("HTTP_STATUS_LENGTH_REQUIRED")
#pragma pop_macro("HTTP_STATUS_GONE")
#pragma pop_macro("HTTP_STATUS_CONFLICT")
#pragma pop_macro("HTTP_STATUS_REQUEST_TIMEOUT")
#pragma pop_macro("HTTP_STATUS_PROXY_AUTH_REQ")
#pragma pop_macro("HTTP_STATUS_NONE_ACCEPTABLE")
#pragma pop_macro("HTTP_STATUS_BAD_METHOD")
#pragma pop_macro("HTTP_STATUS_NOT_FOUND")
#pragma pop_macro("HTTP_STATUS_FORBIDDEN")
#pragma pop_macro("HTTP_STATUS_PAYMENT_REQ")
#pragma pop_macro("HTTP_STATUS_DENIED")
#pragma pop_macro("HTTP_STATUS_BAD_REQUEST")
#pragma pop_macro("HTTP_STATUS_PERMANENT_REDIRECT")
#pragma pop_macro("HTTP_STATUS_REDIRECT_KEEP_VERB")
#pragma pop_macro("HTTP_STATUS_USE_PROXY")
#pragma pop_macro("HTTP_STATUS_NOT_MODIFIED")
#pragma pop_macro("HTTP_STATUS_REDIRECT_METHOD")
#pragma pop_macro("HTTP_STATUS_REDIRECT")
#pragma pop_macro("HTTP_STATUS_MOVED")
#pragma pop_macro("HTTP_STATUS_AMBIGUOUS")
#pragma pop_macro("HTTP_STATUS_WEBDAV_MULTI_STATUS")
#pragma pop_macro("HTTP_STATUS_PARTIAL_CONTENT")
#pragma pop_macro("HTTP_STATUS_RESET_CONTENT")
#pragma pop_macro("HTTP_STATUS_NO_CONTENT")
#pragma pop_macro("HTTP_STATUS_PARTIAL")
#pragma pop_macro("HTTP_STATUS_ACCEPTED")
#pragma pop_macro("HTTP_STATUS_CREATED")
#pragma pop_macro("HTTP_STATUS_OK")
#pragma pop_macro("HTTP_STATUS_SWITCH_PROTOCOLS")
#pragma pop_macro("HTTP_STATUS_CONTINUE")
#pragma pop_macro("DrawState")

namespace JWNU::BinaryHttp
{
	struct FHandle { HINTERNET Value = nullptr; ~FHandle() { if (Value) WinHttpCloseHandle(Value); } };
	inline FString ReadHeader(HINTERNET Request, const TCHAR* Name)
	{
		DWORD Size = 0;
		WinHttpQueryHeaders(Request, WINHTTP_QUERY_CUSTOM, Name, nullptr, &Size, WINHTTP_NO_HEADER_INDEX);
		if (!Size || Size > 1024) return {};
		TArray<TCHAR> Buffer; Buffer.SetNumZeroed(Size / sizeof(TCHAR) + 1);
		return WinHttpQueryHeaders(Request, WINHTTP_QUERY_CUSTOM, Name, Buffer.GetData(), &Size, WINHTTP_NO_HEADER_INDEX) ? FString(Buffer.GetData()) : FString();
	}
}
#endif

FJWNU_BinaryHttpResult FJWNU_BinaryHttp::Post(const FString& Host, const FString& Path, uint16 Port, bool bSecure,
	const TMap<FString, FString>& Headers, const FString& JsonBody, double TimeoutSeconds, int32 MaxResponseBytes, const TAtomic<bool>& Cancelled, const TArray<FString>& ResponseHeaderNames, int32 MaxRequestBytes)
{
    return Execute(TEXT("POST"), Host, Path, Port, bSecure, Headers, JsonBody, TimeoutSeconds, MaxResponseBytes, Cancelled, ResponseHeaderNames, MaxRequestBytes);
}

FJWNU_BinaryHttpResult FJWNU_BinaryHttp::Get(const FString& Host, const FString& Path, uint16 Port, bool bSecure,
    const TMap<FString, FString>& Headers, double TimeoutSeconds, int32 MaxResponseBytes, const TAtomic<bool>& Cancelled)
{
    return Execute(TEXT("GET"), Host, Path, Port, bSecure, Headers, FString(), TimeoutSeconds, MaxResponseBytes, Cancelled, {}, 65536);
}

FJWNU_BinaryHttpResult FJWNU_BinaryHttp::Execute(const TCHAR* Method, const FString& Host, const FString& Path, uint16 Port, bool bSecure,
    const TMap<FString, FString>& Headers, const FString& JsonBody, double TimeoutSeconds, int32 MaxResponseBytes,
    const TAtomic<bool>& Cancelled, const TArray<FString>& ResponseHeaderNames, int32 MaxRequestBytes)
{
	FJWNU_BinaryHttpResult Result;
	if (IsInGameThread() || Host.IsEmpty() || Host.Contains(TEXT("/")) || !Path.StartsWith(TEXT("/"))
		|| (!bSecure && Host != TEXT("127.0.0.1")) || !FMath::IsFinite(TimeoutSeconds) || TimeoutSeconds <= 0 || TimeoutSeconds > 300
		|| MaxResponseBytes <= 0 || MaxResponseBytes > 256 * 1024 * 1024 || MaxRequestBytes <= 0 || MaxRequestBytes > 64 * 1024 * 1024
		|| JsonBody.Len() > MaxRequestBytes || ResponseHeaderNames.Num() > 16)
	{ Result.Error = TEXT("InvalidRequest"); return Result; }
	FTCHARToUTF8 Body(*JsonBody);
	if (Body.Length() > MaxRequestBytes) { Result.Error = TEXT("RequestLimit"); return Result; }
	FString HeaderText = TEXT("Content-Type: application/json\r\nAccept: application/octet-stream\r\n");
	ON_SCOPE_EXIT { if (!HeaderText.IsEmpty()) FMemory::Memzero(HeaderText.GetCharArray().GetData(), HeaderText.GetCharArray().Num() * sizeof(TCHAR)); };
	for (const auto& Pair : Headers)
	{
		if (Pair.Key.IsEmpty() || Pair.Key.Len() > 128 || Pair.Value.Len() > 16384) { Result.Error = TEXT("InvalidHeaders"); return Result; }
		for (TCHAR C : Pair.Key) if (!(FChar::IsAlnum(C) || C == '-')) { Result.Error = TEXT("InvalidHeaders"); return Result; }
		for (TCHAR C : Pair.Value) if (C < 32 || C >= 127) { Result.Error = TEXT("InvalidHeaders"); return Result; }
		HeaderText += Pair.Key + TEXT(": ") + Pair.Value + TEXT("\r\n");
	}
#if PLATFORM_WINDOWS
	using namespace JWNU::BinaryHttp;
	const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
	FHandle Session{WinHttpOpen(L"JWNU-BinaryHttp/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
	if (!Session.Value) { Result.Error = TEXT("Network"); return Result; }
	FHandle Connection{WinHttpConnect(Session.Value, *Host, Port, 0)};
	if (!Connection.Value) { Result.Error = TEXT("Network"); return Result; }
	FHandle Request{WinHttpOpenRequest(Connection.Value, Method, *Path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, bSecure ? WINHTTP_FLAG_SECURE : 0)};
	if (!Request.Value) { Result.Error = TEXT("Network"); return Result; }
	DWORD Redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
	DWORD Disable = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
	if (!WinHttpSetOption(Request.Value, WINHTTP_OPTION_REDIRECT_POLICY, &Redirect, sizeof(Redirect))
		|| !WinHttpSetOption(Request.Value, WINHTTP_OPTION_DISABLE_FEATURE, &Disable, sizeof(Disable))
		|| !WinHttpSetTimeouts(Request.Value, 5000, 5000, 10000, static_cast<int32>(TimeoutSeconds * 1000)))
	{ Result.Error = TEXT("Network"); return Result; }
	if (Cancelled.Load()) { Result.Error = TEXT("Cancelled"); return Result; }
	if (!WinHttpSendRequest(Request.Value, *HeaderText, HeaderText.Len(), const_cast<ANSICHAR*>(Body.Get()), Body.Length(), Body.Length(), 0)
		|| !WinHttpReceiveResponse(Request.Value, nullptr))
	{ Result.Error = GetLastError() == ERROR_WINHTTP_TIMEOUT ? TEXT("Timeout") : TEXT("Network"); return Result; }
	DWORD Status = 0, Size = sizeof(Status);
	if (!WinHttpQueryHeaders(Request.Value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &Status, &Size, WINHTTP_NO_HEADER_INDEX))
	{ Result.Error = TEXT("Network"); return Result; }
	Result.StatusCode = static_cast<int32>(Status);
	for (const FString& Name : ResponseHeaderNames) Result.Headers.Add(Name.ToLower(), ReadHeader(Request.Value, *Name));
	for (;;)
	{
		if (Cancelled.Load()) { Result.Bytes.Reset(); Result.Error = TEXT("Cancelled"); return Result; }
		const double Remaining = Deadline - FPlatformTime::Seconds();
		if (Remaining <= 0) { Result.Bytes.Reset(); Result.Error = TEXT("Timeout"); return Result; }
		WinHttpSetTimeouts(Request.Value, 5000, 5000, 10000, FMath::Max(1, static_cast<int32>(Remaining * 1000)));
		uint8 Buffer[8192]; DWORD Count = 0;
		if (!WinHttpReadData(Request.Value, Buffer, sizeof(Buffer), &Count)) { Result.Error = TEXT("Network"); Result.Bytes.Reset(); return Result; }
		if (!Count) break;
		if (static_cast<int64>(Result.Bytes.Num()) + Count > MaxResponseBytes) { Result.Error = TEXT("ResponseLimit"); Result.Bytes.Reset(); return Result; }
		Result.Bytes.Append(Buffer, Count);
	}
#else
	Result.Error = TEXT("UnsupportedPlatform");
#endif
	return Result;
}
