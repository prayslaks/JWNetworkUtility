// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_TypeSafe.h"
#include "JWNU_TypeSafeRequest.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

void UJWNU_GIS_TypeSafe::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJWNU_GIS_TypeSafe::Tick));
    Cleanup = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UJWNU_GIS_TypeSafe::WorldCleanup);
}
void UJWNU_GIS_TypeSafe::Deinitialize()
{
    bStopping = true;
    FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
    FWorldDelegates::OnWorldCleanup.Remove(Cleanup);
    TArray<TStrongObjectPtr<UJWNU_TypeSafeRequest>> Snapshot;
    for (auto Request : Active) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot) { Request->Cancel(); }
    Active.Reset();
    Super::Deinitialize();
}
bool UJWNU_GIS_TypeSafe::Track(UJWNU_TypeSafeRequest* Request)
{
    if (bStopping || !Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { return false; }
    Active.AddUnique(Request); return true;
}
bool UJWNU_GIS_TypeSafe::Tick(float DeltaSeconds)
{
    TArray<TStrongObjectPtr<UJWNU_TypeSafeRequest>> Snapshot;
    for (auto Request : Active) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot)
    {
        if (!Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { Request->Cancel(); }
        else { Request->Pump(); }
    }
    Active.RemoveAll([](const auto& Request) { return !Request->IsActive(); });
    return true;
}
void UJWNU_GIS_TypeSafe::WorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    TArray<TStrongObjectPtr<UJWNU_TypeSafeRequest>> Snapshot;
    for (auto Request : Active) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot)
    { if (Request->OwnerWorld.Get() == World) { Request->Cancel(); } }
}
