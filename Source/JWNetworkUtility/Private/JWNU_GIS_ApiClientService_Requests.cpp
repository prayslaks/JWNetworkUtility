// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_ApiClientService.h"
#include "JWNU_ApiRequest.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

void UJWNU_GIS_ApiClientService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RequestTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJWNU_GIS_ApiClientService::TickRequests));
    RequestWorldCleanup = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UJWNU_GIS_ApiClientService::CleanupRequestWorld);
}

void UJWNU_GIS_ApiClientService::Deinitialize()
{
    bStoppingRequests = true;
    FTSTicker::GetCoreTicker().RemoveTicker(RequestTicker);
    FWorldDelegates::OnWorldCleanup.Remove(RequestWorldCleanup);
    TArray<TStrongObjectPtr<UJWNU_ApiRequest>> Snapshot;
    for (auto Request : ActiveRequests) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot) { Request->Cancel(); }
    ActiveRequests.Reset();
    // 갱신 대기 콜백의 강한 핸들 참조가 GameInstance 종료 이후 남지 않게 한다.
    for (const auto& Pair : ActiveRefreshJobs) { if (Pair.Value) { Pair.Value->Cancel(); } }
    ActiveRefreshJobs.Reset();
    PendingJobQueues.Reset();
    RefreshInProgressFlags.Reset();
    Super::Deinitialize();
}

bool UJWNU_GIS_ApiClientService::TrackRequest(UJWNU_ApiRequest* Request)
{
    if (bStoppingRequests || !Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { return false; }
    ActiveRequests.AddUnique(Request); return true;
}

bool UJWNU_GIS_ApiClientService::TickRequests(float DeltaSeconds)
{
    TArray<TStrongObjectPtr<UJWNU_ApiRequest>> Snapshot;
    for (auto Request : ActiveRequests) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot)
    {
        if (!Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { Request->Cancel(); }
    }
    ActiveRequests.RemoveAll([](const auto& Request) { return !Request->IsActive(); });
    return true;
}

void UJWNU_GIS_ApiClientService::CleanupRequestWorld(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    TArray<TStrongObjectPtr<UJWNU_ApiRequest>> Snapshot;
    for (auto Request : ActiveRequests) { Snapshot.Emplace(Request); }
    for (const auto& Request : Snapshot)
    { if (Request->OwnerWorld.Get() == World) { Request->Cancel(); } }
}
