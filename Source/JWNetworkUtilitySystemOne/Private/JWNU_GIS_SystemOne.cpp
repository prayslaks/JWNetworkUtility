// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_GIS_SystemOne.h"
#include "JWNU_SystemOneRequest.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

void UJWNU_GIS_SystemOne::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJWNU_GIS_SystemOne::Tick));
    Cleanup = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UJWNU_GIS_SystemOne::WorldCleanup);
}
void UJWNU_GIS_SystemOne::Deinitialize()
{
    bStopping = true;
    FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
    FWorldDelegates::OnWorldCleanup.Remove(Cleanup);
    TArray<TStrongObjectPtr<UJWNU_SystemOneRequest>> Snapshot;
    for (auto Request : Active) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot) { Request->Cancel(); }
    Active.Reset();
    Super::Deinitialize();
}
bool UJWNU_GIS_SystemOne::Track(UJWNU_SystemOneRequest* Request)
{
    if (bStopping || !Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { return false; }
    Active.AddUnique(Request); return true;
}
bool UJWNU_GIS_SystemOne::Tick(float DeltaSeconds)
{
    TArray<TStrongObjectPtr<UJWNU_SystemOneRequest>> Snapshot;
    for (auto Request : Active) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot)
    {
        if (!Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { Request->Cancel(); }
        else { Request->Pump(); }
    }
    Active.RemoveAll([](const auto& Request) { return !Request->IsActive(); });
    return true;
}
void UJWNU_GIS_SystemOne::WorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    TArray<TStrongObjectPtr<UJWNU_SystemOneRequest>> Snapshot;
    for (auto Request : Active) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot)
    { if (Request->OwnerWorld.Get() == World) { Request->Cancel(); } }
}
