// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_OpenAI.h"
#include "JWNU_OpenAILiveSession.h"
#include "JWNU_OpenAITranscriptionSession.h"
#include "JWNU_GIS_WebSocketClient.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"

void UJWNU_GIS_OpenAI::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UJWNU_GIS_WebSocketClient>();
    Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJWNU_GIS_OpenAI::Tick));
    Cleanup = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UJWNU_GIS_OpenAI::WorldCleanup);
}
void UJWNU_GIS_OpenAI::Deinitialize()
{
    bStopping = true;
    FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
    FWorldDelegates::OnWorldCleanup.Remove(Cleanup);
    TArray<TStrongObjectPtr<UJWNU_OpenAILiveSession>> Snapshot;
    for (auto Session : Active) { Snapshot.Emplace(Session); }
    for (const auto& Session : Snapshot) { Session->Cancel(); }
    Active.Reset();
    TArray<TStrongObjectPtr<UJWNU_OpenAITranscriptionSession>> Transcriptions;
    for (auto Session : ActiveTranscriptions) { Transcriptions.Emplace(Session); }
    for (const auto& Session : Transcriptions) { Session->Cancel(); }
    ActiveTranscriptions.Reset();
    Super::Deinitialize();
}
bool UJWNU_GIS_OpenAI::Track(UJWNU_OpenAILiveSession* Session)
{
    if (bStopping || !Session->OwnerWorld.IsValid() || Session->OwnerWorld->bIsTearingDown) { return false; }
    Active.AddUnique(Session);
    return true;
}
bool UJWNU_GIS_OpenAI::Tick(float DeltaSeconds)
{
    TArray<TStrongObjectPtr<UJWNU_OpenAILiveSession>> Snapshot;
    for (auto Session : Active) { Snapshot.Emplace(Session); }
    for (const auto& Session : Snapshot)
    {
        if (!Session->OwnerWorld.IsValid() || Session->OwnerWorld->bIsTearingDown) { Session->Cancel(); }
        else { Session->Pump(); }
    }
    Active.RemoveAll([](const auto& Session) { return !Session->IsActive(); });
    TArray<TStrongObjectPtr<UJWNU_OpenAITranscriptionSession>> Transcriptions;
    for (auto Session : ActiveTranscriptions) { Transcriptions.Emplace(Session); }
    for (const auto& Session : Transcriptions)
    {
        if (!Session->OwnerWorld.IsValid() || Session->OwnerWorld->bIsTearingDown) { Session->Cancel(); }
        else { Session->Pump(); }
    }
    ActiveTranscriptions.RemoveAll([](const auto& Session) { return !Session->IsActive(); });
    return true;
}
void UJWNU_GIS_OpenAI::WorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    TArray<TStrongObjectPtr<UJWNU_OpenAILiveSession>> Snapshot;
    for (auto Session : Active) { Snapshot.Emplace(Session); }
    for (const auto& Session : Snapshot)
    {
        if (Session->OwnerWorld.Get() == World) { Session->Cancel(); }
    }
    TArray<TStrongObjectPtr<UJWNU_OpenAITranscriptionSession>> Transcriptions;
    for (auto Session : ActiveTranscriptions) { Transcriptions.Emplace(Session); }
    for (const auto& Session : Transcriptions)
    {
        if (Session->OwnerWorld.Get() == World) { Session->Cancel(); }
    }
}

bool UJWNU_GIS_OpenAI::Track(UJWNU_OpenAITranscriptionSession* Session)
{
    if (bStopping || !Session->OwnerWorld.IsValid() || Session->OwnerWorld->bIsTearingDown) { return false; }
    ActiveTranscriptions.AddUnique(Session);
    return true;
}
