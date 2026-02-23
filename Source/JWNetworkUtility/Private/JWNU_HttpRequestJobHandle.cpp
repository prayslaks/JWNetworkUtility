// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_HttpRequestJobHandle.h"

void UJWNU_HttpRequestJobHandle::Cancel()
{
	bIsCancelled = true;
	bIsWaitingForRefresh = false;

	if (CurrentJob && CurrentJob->IsRunning())
	{
		CurrentJob->Cancel();
	}
}

bool UJWNU_HttpRequestJobHandle::IsRunning() const
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
