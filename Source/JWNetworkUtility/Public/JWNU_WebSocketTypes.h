// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNU_WebSocketTypes.generated.h"

/** 연결 핸들의 현재 상태다. */
UENUM(BlueprintType)
enum class EJWNU_WebSocketState : uint8 { Idle, Connecting, Connected, Closing, Closed, Failed };

/** 연결 또는 수신 실패 원인이다. */
UENUM(BlueprintType, meta=(ScriptName="JWNU_WebSocketErrorCode"))
enum class EJWNU_WebSocketError : uint8 { None, InvalidConfiguration, Connection, Timeout, BufferLimit };

/** 연결 실패 진단 정보다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_WebSocketError
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="JWNU|WebSocket") EJWNU_WebSocketError Code = EJWNU_WebSocketError::None;
	UPROPERTY(BlueprintReadOnly, Category="JWNU|WebSocket") FString Message;
};

/** 연결 종료 정보다. 1006은 로컬에서 합성한 비정상 종료 코드다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_WebSocketCloseInfo
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category="JWNU|WebSocket") int32 Code = 1000;
	UPROPERTY(BlueprintReadOnly, Category="JWNU|WebSocket") FString Reason;
	UPROPERTY(BlueprintReadOnly, Category="JWNU|WebSocket") bool bWasClean = false;
	UPROPERTY(BlueprintReadOnly, Category="JWNU|WebSocket") bool bWasLocal = false;
};

/** 연결 협상과 메시지 크기 제한을 정의한다. 유휴 연결은 자동으로 닫지 않는다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_WebSocketOptions
{
	GENERATED_BODY()
	/** Authorization 등 연결 협상에 사용할 추가 헤더 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|WebSocket") TMap<FString, FString> Headers;
	/** 서버와 협상할 서브프로토콜 목록 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|WebSocket") TArray<FString> Protocols;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|WebSocket", meta=(ClampMin="0")) float ConnectTimeoutSeconds = 15.f;
	/** 종료 응답 대기 상한이며 응답이 없으면 비정상 종료로 정리하는 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|WebSocket", meta=(ClampMin="0.1")) float CloseTimeoutSeconds = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|WebSocket", meta=(ClampMin="1")) int32 MaxMessageBytes = 1048576;
	/** 래퍼의 수신 큐 상한이며 엔진 내부 버퍼와 별개인 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|WebSocket", meta=(ClampMin="1")) int32 MaxQueuedBytes = 4194304;
	/** 아직 라이브러리에 전달하지 않은 송신 메시지의 바이트·항목 비용 상한 필드. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|WebSocket", meta=(ClampMin="1")) int32 MaxSendQueuedBytes = 4194304;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJWNU_WebSocketConnectedBP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_WebSocketTextBP, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_WebSocketBinaryBP, const TArray<uint8>&, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_WebSocketErrorBP, const FJWNU_WebSocketError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_WebSocketClosedBP, const FJWNU_WebSocketCloseInfo&, Info);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_WebSocketTextNative, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_WebSocketBinaryNative, const TArray<uint8>&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_WebSocketErrorNative, const FJWNU_WebSocketError&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_WebSocketClosedNative, const FJWNU_WebSocketCloseInfo&);
