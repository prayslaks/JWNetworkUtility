// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "JWNU_BFL_SystemOne.h"
#include "JWNU_SystemOneRequest.h"

namespace JWNU::SystemOneBFL
{
inline UJWNU_SystemOneRequest* CreateBoundRequest(const UObject* WorldContextObject,
    const FJWNU_SystemOneCompletedCallback& OnCompleted, const FJWNU_SystemOneFailedCallback& OnFailed)
{
    auto* Request = UJWNU_SystemOneRequest::CreateSystemOneRequest(WorldContextObject);
    if (!Request)
    {
        FJWNU_SystemOneError Error; Error.Code = EJWNU_SystemOneErrorCode::Configuration;
        Error.Message = TEXT("Cannot create SystemOne request for this world.");
        OnFailed.ExecuteIfBound(Error);
        return nullptr;
    }
    Request->OnCompletedNative.AddLambda([OnCompleted](const FJWNU_SystemOneResult& Result) { OnCompleted.ExecuteIfBound(Result); });
    Request->OnFailedNative.AddLambda([OnFailed](const FJWNU_SystemOneError& Error) { OnFailed.ExecuteIfBound(Error); });
    return Request;
}
}
