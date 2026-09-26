// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_HttpRequestJobHandle.h"

void UJWNU_HttpRequestJobHandle::Cancel()
{
	if (bIsCancelled) { return; }
	const bool bWasWaiting = bIsWaitingForRefresh;
	bIsCancelled = true;
	bIsWaitingForRefresh = false;

	if (CurrentJob && CurrentJob->IsRunning())
	{
		CurrentJob->Cancel();
	}
	else if (bWasWaiting) { SsePendingCancel.ExecuteIfBound(); }
}

bool UJWNU_HttpRequestJobHandle::IsActive() const
{
	if (bIsCancelled)
	{
		return false;
	}

	return (CurrentJob && CurrentJob->IsRunning()) || bIsWaitingForRefresh;
}

bool UJWNU_HttpRequestJobHandle::IsCancelled() const
{
	return bIsCancelled;
}

void UJWNU_HttpRequestJobHandle::BindJob(UJWNU_HttpRequestJob* InJob)
{
	CurrentJob = InJob;
}

void UJWNU_HttpRequestJobHandle::MarkWaitingForRefresh()
{
	bIsWaitingForRefresh = true;
}

void UJWNU_HttpRequestJobHandle::ClearWaitingForRefresh()
{
	bIsWaitingForRefresh = false;
}
