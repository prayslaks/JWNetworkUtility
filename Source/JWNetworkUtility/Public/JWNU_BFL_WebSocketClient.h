// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_WebSocketConnection.h"
#include "JWNU_BFL_WebSocketClient.generated.h"

/** WebSocket 연결 핸들을 만드는 Blueprint 진입점이다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_BFL_WebSocketClient : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Idle 핸들을 생성하는 함수. 이벤트를 바인딩한 뒤 핸들의 Connect를 호출한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|WebSocket", meta=(WorldContext="WorldContextObject"))
	static UJWNU_WebSocketConnection* CreateWebSocketConnection(const UObject* WorldContextObject);
	/** 핸들을 반환하고 다음 Tick에 연결을 시작하는 함수. 반환 핸들에 이벤트를 바인딩한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|WebSocket", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="Options"))
	static UJWNU_WebSocketConnection* ConnectWebSocket(const UObject* WorldContextObject, const FString& URL, const FJWNU_WebSocketOptions& Options);
};
