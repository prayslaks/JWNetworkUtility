// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_WebSocketClient.h"
#include "JWNU_WebSocketConnection.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

void UJWNU_GIS_WebSocketClient::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bShuttingDown = false;
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJWNU_GIS_WebSocketClient::Tick));
	CleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UJWNU_GIS_WebSocketClient::OnWorldCleanup);
}

void UJWNU_GIS_WebSocketClient::Deinitialize()
{
	bShuttingDown = true;
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	FWorldDelegates::OnWorldCleanup.Remove(CleanupHandle);
	TArray<TStrongObjectPtr<UJWNU_WebSocketConnection>> Snapshot;
	for (auto Connection : ActiveConnections) { Snapshot.Emplace(Connection); }
	for (const auto& Connection : Snapshot) { Connection->Shutdown(); }
	ActiveConnections.Reset();
	Super::Deinitialize();
}

UJWNU_WebSocketConnection* UJWNU_GIS_WebSocketClient::CreateConnection(const UObject* Context)
{
	check(IsInGameThread());
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || World->bIsTearingDown || !World->GetGameInstance()) { return nullptr; }
	auto* Self = World->GetGameInstance()->GetSubsystem<UJWNU_GIS_WebSocketClient>();
	if (!Self || Self->bShuttingDown) { return nullptr; }
	auto* Connection = NewObject<UJWNU_WebSocketConnection>(Self);
	Connection->OwnerClient = Self;
	Connection->OwnerWorld = World;
	return Connection;
}

bool UJWNU_GIS_WebSocketClient::Track(UJWNU_WebSocketConnection* Connection)
{
	if (bShuttingDown || !Connection->OwnerWorld.IsValid() || Connection->OwnerWorld->bIsTearingDown) { return false; }
	ActiveConnections.AddUnique(Connection);
	return true;
}

bool UJWNU_GIS_WebSocketClient::Tick(float DeltaSeconds)
{
	TArray<TStrongObjectPtr<UJWNU_WebSocketConnection>> Snapshot;
	for (auto Connection : ActiveConnections) { Snapshot.Emplace(Connection); }
	for (const auto& Connection : Snapshot)
	{
		if (!Connection->OwnerWorld.IsValid() || Connection->OwnerWorld->bIsTearingDown) { Connection->Shutdown(); }
		else { Connection->Pump(); }
	}
	ActiveConnections.RemoveAll([](const auto& Connection) { return !Connection->IsActive(); });
	return true;
}

void UJWNU_GIS_WebSocketClient::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	TArray<TStrongObjectPtr<UJWNU_WebSocketConnection>> Snapshot;
	for (auto Connection : ActiveConnections) { Snapshot.Emplace(Connection); }
	for (const auto& Connection : Snapshot)
	{
		if (Connection->OwnerWorld.Get() == World) { Connection->Shutdown(); }
	}
}
