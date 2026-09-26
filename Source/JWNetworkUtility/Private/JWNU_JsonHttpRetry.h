// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "String/LexFromString.h"

namespace JWNU::JsonHttp
{
/** Retry-After 초·HTTP 날짜와 밀리초 힌트를 해석한다. */
inline double RetryDelay(const TMap<FString, FString>& Headers, double Backoff)
{
    double Seconds = 0;
    const FString* Milliseconds = Headers.Find(TEXT("retry-after-ms"));
    if (Milliseconds && LexTryParseString(Seconds, **Milliseconds) && FMath::IsFinite(Seconds) && Seconds >= 0)
    { return FMath::Max(Backoff, Seconds / 1000.); }
    if (const FString* Value = Headers.Find(TEXT("retry-after")))
    {
        if (LexTryParseString(Seconds, **Value) && FMath::IsFinite(Seconds) && Seconds >= 0)
        { return FMath::Max(Backoff, Seconds); }
        FDateTime Date;
        if (FDateTime::ParseHttpDate(*Value, Date))
        { return FMath::Max(Backoff, (Date - FDateTime::UtcNow()).GetTotalSeconds()); }
    }
    return Backoff;
}
}
