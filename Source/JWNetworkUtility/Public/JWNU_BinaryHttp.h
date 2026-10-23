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
	/** 동기 호출이므로 게임 스레드에서 실행하지 않는다. HTTPS 또는 명시적 loopback만 허용하며 redirect는 따르지 않는다. */
	static FJWNU_BinaryHttpResult Post(const FString& Host, const FString& Path, uint16 Port, bool bSecure,
		const TMap<FString, FString>& Headers, const FString& JsonBody, double TimeoutSeconds, int32 MaxResponseBytes, const TAtomic<bool>& Cancelled,
		const TArray<FString>& ResponseHeaderNames = {});
};
