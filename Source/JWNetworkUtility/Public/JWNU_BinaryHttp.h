// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
#pragma once
#include "CoreMinimal.h"

/** 네이티브 유한 이진 POST의 결과. 응답 본문은 호출자가 해석하며 자동 재시도하지 않는다. */
struct JWNETWORKUTILITY_API FJWNU_BinaryHttpResult
{
	int32 StatusCode = 0;
	FString Error;
	TMap<FString, FString> Headers;
	TArray<uint8> Bytes;
};

/** 에디터 등 World가 없는 호출자가 작업 스레드에서 사용하는 이진 HTTP 전송이다. */
struct JWNETWORKUTILITY_API FJWNU_BinaryHttp
{
    /** 작업 스레드에서 본문 없는 GET을 수행하는 함수. Post와 같은 상한·redirect 금지를 적용한다. */
    static FJWNU_BinaryHttpResult Get(const FString& Host, const FString& Path, uint16 Port, bool bSecure,
        const TMap<FString, FString>& Headers, double TimeoutSeconds, int32 MaxResponseBytes, const TAtomic<bool>& Cancelled);
	/** 작업 스레드 전용. HTTPS/loopback, UTF-8 요청 기본 64KiB/최대 64MiB, 응답 최대 256MiB. redirect·자동 재시도는 없다. */
	static FJWNU_BinaryHttpResult Post(const FString& Host, const FString& Path, uint16 Port, bool bSecure,
		const TMap<FString, FString>& Headers, const FString& JsonBody, double TimeoutSeconds, int32 MaxResponseBytes, const TAtomic<bool>& Cancelled,
		const TArray<FString>& ResponseHeaderNames = {}, int32 MaxRequestBytes = 65536);
private:
    static FJWNU_BinaryHttpResult Execute(const TCHAR* Method, const FString& Host, const FString& Path, uint16 Port, bool bSecure,
        const TMap<FString, FString>& Headers, const FString& JsonBody, double TimeoutSeconds, int32 MaxResponseBytes,
        const TAtomic<bool>& Cancelled, const TArray<FString>& ResponseHeaderNames, int32 MaxRequestBytes);
};
