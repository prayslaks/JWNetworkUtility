// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_RequestBase.h"
#include "UObject/StrongObjectPtr.h"

void UJWNU_RequestBase::Cancel()
{
    check(IsInGameThread());
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_RequestBase> KeepAlive(this);
    CancelRequest();
}

bool UJWNU_RequestBase::ActivateRequest()
{
    check(IsInGameThread());
    if (RequestState != EJWNU_RequestState::Created) { return false; }
    RequestState = EJWNU_RequestState::Active;
    return true;
}

bool UJWNU_RequestBase::SetFinishedState(EJWNU_RequestState TerminalState)
{
    check(IsInGameThread());
    check(TerminalState == EJWNU_RequestState::Succeeded || TerminalState == EJWNU_RequestState::Failed || TerminalState == EJWNU_RequestState::Cancelled);
    if (!IsActive()) { return false; }
    RequestState = TerminalState;
    return true;
}

void UJWNU_RequestBase::BroadcastFinished()
{
    check(IsInGameThread());
    if (RequestState == EJWNU_RequestState::Created || IsActive() || bFinishedBroadcast) { return; }
    TStrongObjectPtr<UJWNU_RequestBase> KeepAlive(this);
    bFinishedBroadcast = true;
    OnFinishedNative.Broadcast(this, RequestState);
    OnFinished.Broadcast(this, RequestState);
}
