// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "JWNU_GIS_WebSocketClient.generated.h"

class UJWNU_WebSocketConnection;

/** 실행 중인 WebSocket 연결을 보관하고 해당 월드가 정리될 때 종료한다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_WebSocketClient : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/** 연결 전 이벤트를 바인딩할 수 있는 Idle 핸들을 생성하는 함수. */
	static UJWNU_WebSocketConnection* CreateConnection(const UObject* WorldContextObject);
private:
	friend class UJWNU_WebSocketConnection;
	bool Track(UJWNU_WebSocketConnection* Connection);
	bool Tick(float DeltaSeconds);
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	UPROPERTY() TArray<TObjectPtr<UJWNU_WebSocketConnection>> ActiveConnections;
	FTSTicker::FDelegateHandle TickerHandle;
	FDelegateHandle CleanupHandle;
	bool bShuttingDown = false;
};
