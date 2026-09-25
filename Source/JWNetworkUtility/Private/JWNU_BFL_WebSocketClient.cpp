// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_BFL_WebSocketClient.h"
#include "JWNU_GIS_WebSocketClient.h"

UJWNU_WebSocketConnection* UJWNU_BFL_WebSocketClient::CreateWebSocketConnection(const UObject* WorldContextObject)
{
	return UJWNU_GIS_WebSocketClient::CreateConnection(WorldContextObject);
}

UJWNU_WebSocketConnection* UJWNU_BFL_WebSocketClient::ConnectWebSocket(const UObject* WorldContextObject, const FString& URL, const FJWNU_WebSocketOptions& Options)
{
	auto* Connection = CreateWebSocketConnection(WorldContextObject);
	return Connection && Connection->Connect(URL, Options) ? Connection : nullptr;
}
