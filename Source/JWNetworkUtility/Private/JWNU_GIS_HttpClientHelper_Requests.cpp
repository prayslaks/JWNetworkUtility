// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_HttpClientHelper.h"
#include "JWNU_HttpRequest.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

void UJWNU_GIS_HttpClientHelper::Deinitialize()
{
    bStoppingRequests = true;
    FTSTicker::GetCoreTicker().RemoveTicker(RequestTicker);
    FWorldDelegates::OnWorldCleanup.Remove(RequestWorldCleanup);
    TArray<TStrongObjectPtr<UJWNU_HttpRequest>> Snapshot;
    for (auto Request : ActiveRequests) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot) { Request->Cancel(); }
    ActiveRequests.Reset();
    Super::Deinitialize();
}

bool UJWNU_GIS_HttpClientHelper::TrackRequest(UJWNU_HttpRequest* Request)
{
    if (bStoppingRequests || !Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { return false; }
    ActiveRequests.AddUnique(Request); return true;
}

bool UJWNU_GIS_HttpClientHelper::TickRequests(float DeltaSeconds)
{
    TArray<TStrongObjectPtr<UJWNU_HttpRequest>> Snapshot;
    for (auto Request : ActiveRequests) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot)
    { if (!Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { Request->Cancel(); } }
    ActiveRequests.RemoveAll([](const auto& Request) { return !Request->IsActive(); });
    return true;
}

void UJWNU_GIS_HttpClientHelper::CleanupRequestWorld(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    TArray<TStrongObjectPtr<UJWNU_HttpRequest>> Snapshot;
    for (auto Request : ActiveRequests) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot)
    { if (Request->OwnerWorld.Get() == World) { Request->Cancel(); } }
}
