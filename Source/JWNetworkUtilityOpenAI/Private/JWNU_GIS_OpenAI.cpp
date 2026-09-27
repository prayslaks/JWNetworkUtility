// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_GIS_OpenAI.h"
#include "JWNU_OpenAILiveSession.h"
#include "JWNU_OpenAITranscriptionSession.h"
#include "JWNU_GptLiveTranscriptor.h"
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
    // 상위 전사기를 먼저 취소해 하위 세션의 종료가 새 오류·결과 콜백으로 전달되지 않게 한다.
    TArray<TStrongObjectPtr<UJWNU_GptLiveTranscriptor>> Transcriptors;
    for (auto Transcriptor : ActiveTranscriptors) { Transcriptors.Emplace(Transcriptor); }
    for (const auto& Transcriptor : Transcriptors) { Transcriptor->Cancel(); }
    ActiveTranscriptors.Reset();
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
    TArray<TStrongObjectPtr<UJWNU_GptLiveTranscriptor>> Transcriptors;
    for (auto Transcriptor : ActiveTranscriptors) { Transcriptors.Emplace(Transcriptor); }
    for (const auto& Transcriptor : Transcriptors)
    {
        if (!Transcriptor->OwnerWorld.IsValid() || Transcriptor->OwnerWorld->bIsTearingDown) { Transcriptor->Cancel(); }
        else { Transcriptor->Poll(FPlatformTime::Seconds()); }
    }
    ActiveTranscriptors.RemoveAll([](const auto& Transcriptor) { return !Transcriptor->IsActive(); });
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
    TArray<TStrongObjectPtr<UJWNU_GptLiveTranscriptor>> Transcriptors;
    for (auto Transcriptor : ActiveTranscriptors) { Transcriptors.Emplace(Transcriptor); }
    for (const auto& Transcriptor : Transcriptors)
    {
        if (Transcriptor->OwnerWorld.Get() == World) { Transcriptor->Cancel(); }
    }
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

bool UJWNU_GIS_OpenAI::Track(UJWNU_GptLiveTranscriptor* Transcriptor)
{
    if (bStopping || !Transcriptor->OwnerWorld.IsValid() || Transcriptor->OwnerWorld->bIsTearingDown) { return false; }
    ActiveTranscriptors.AddUnique(Transcriptor);
    return true;
}
