// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_SystemOneRequest.h"
#include "JWNU_SystemOneCodec.h"
#include "JWNU_GIS_SystemOne.h"
#include "JWNU_JsonHttpJob.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Internationalization/Regex.h"
#include "UObject/StrongObjectPtr.h"
#include "Containers/StringConv.h"

UJWNU_SystemOneRequest* UJWNU_SystemOneRequest::CreateSystemOneRequest(const UObject* WorldContextObject)
{
    check(IsInGameThread());
    auto* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World || World->bIsTearingDown || !World->GetGameInstance()) { return nullptr; }
    auto* Client = World->GetGameInstance()->GetSubsystem<UJWNU_GIS_SystemOne>();
    if (!Client || Client->bStopping) { return nullptr; }
    auto* Request = NewObject<UJWNU_SystemOneRequest>(Client);
    Request->OwnerClient = Client; Request->OwnerWorld = World;
    return Request;
}

bool UJWNU_SystemOneRequest::Start(const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options, const FString& ApiKey)
{
    return Schedule(State, Questions, Options, ApiKey, TEXT(""));
}

bool UJWNU_SystemOneRequest::StartFromEnvironment(const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options)
{
    check(IsInGameThread());
    const bool bOpenRouter = Options.Endpoint == TEXT("https://openrouter.ai/api/alpha/decisions")
        || Options.Endpoint == TEXT("https://openrouter.ai/api/v1/systemone");
    const bool bTypeSafe = Options.Endpoint == TEXT("https://api.typesafe.ai/v1/systemone");
    if (!bOpenRouter && !bTypeSafe)
    { return Schedule(State, Questions, Options, TEXT(""), TEXT("Environment API keys require an exact official OpenRouter or TypeSafe endpoint.")); }
    const TCHAR* Variable = bOpenRouter ? TEXT("OPENROUTER_API_KEY") : TEXT("TYPESAFE_API_KEY");
    FString Key = FPlatformMisc::GetEnvironmentVariable(Variable);
    const bool bScheduled = Schedule(State, Questions, Options, Key, Key.IsEmpty() ? FString(Variable) + TEXT(" is not set in this process.") : TEXT(""));
    if (!Key.IsEmpty()) { FMemory::Memzero(Key.GetCharArray().GetData(), Key.GetCharArray().Num() * sizeof(TCHAR)); Key.Reset(); }
    return bScheduled;
}

bool UJWNU_SystemOneRequest::Schedule(const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions, const FJWNU_SystemOneOptions& Options, const FString& ApiKey, const FString& ApiKeyError)
{
    check(IsInGameThread());
    if (GetState() != EJWNU_RequestState::Created || !OwnerClient.IsValid() || !OwnerClient->Track(this)) { return false; }
    ActivateRequest();
    if (!ApiKeyError.IsEmpty())
    { Error.Code = EJWNU_SystemOneErrorCode::Credentials; Error.Message = ApiKeyError; return false; }
    // HTTP는 키 없는 정확한 loopback 주소에만 허용한다. userinfo·fragment·제어문자는 거부한다.
    FRegexMatcher Local(FRegexPattern(TEXT("^http://(127\\.0\\.0\\.1|localhost):([0-9]{1,5})/[^\\s#@]*$")), Options.Endpoint);
    FRegexMatcher Secure(FRegexPattern(TEXT("^https://[A-Za-z0-9.-]+(:[0-9]{1,5})?/[^\\s#@]*$")), Options.Endpoint);
    const bool bLocal = Local.FindNext();
    const bool bSecure = Secure.FindNext();
    if ((!bSecure && !(bLocal && ApiKey.IsEmpty())) || Options.Model.TrimStartAndEnd().IsEmpty()
        || !FMath::IsFinite(Options.TimeoutSeconds) || Options.TimeoutSeconds <= 0
        || !FMath::IsFinite(Options.AttemptTimeoutSeconds) || Options.AttemptTimeoutSeconds <= 0
        || !FMath::IsFinite(Options.InitialRetrySeconds) || Options.InitialRetrySeconds <= 0
        || !FMath::IsFinite(Options.MaxRetrySeconds) || Options.MaxRetrySeconds < Options.InitialRetrySeconds
        || Options.MaxRetries < 0 || Options.MaxRetries > 10 || Options.MaxResponseBytes < 1 || Options.MaxResponseBytes > 16 * 1024 * 1024)
    { Error.Code = EJWNU_SystemOneErrorCode::Configuration; Error.Message = TEXT("Invalid endpoint, model, timeout, retry policy or response limit."); return false; }
    if (!bLocal && ApiKey.IsEmpty())
    { Error.Code = EJWNU_SystemOneErrorCode::Credentials; Error.Message = TEXT("An API key is required."); return false; }
    for (TCHAR Character : ApiKey)
    {
        if (Character <= 32 || Character >= 127)
        { Error.Code = EJWNU_SystemOneErrorCode::Credentials; Error.Message = TEXT("Invalid API key characters."); return false; }
    }
    FString Body, Diagnostic;
    if (!FJWNU_SystemOneCodec::BuildRequest(State, Questions, Options.Model, Body, Diagnostic))
    { Error.Code = EJWNU_SystemOneErrorCode::Configuration; Error.Message = Diagnostic; return false; }
    if (FTCHARToUTF8(*Body).Length() > 2 * 1024 * 1024)
    { Error.Code = EJWNU_SystemOneErrorCode::Configuration; Error.Message = TEXT("Request exceeds the 2 MiB byte limit (not a token budget)."); return false; }
    SubmittedQuestions = Questions;
    FJWNU_JsonHttpOptions Policy;
    Policy.TotalTimeoutSeconds = Options.TimeoutSeconds; Policy.AttemptTimeoutSeconds = Options.AttemptTimeoutSeconds;
    Policy.MaxRetries = Options.MaxRetries; Policy.InitialRetrySeconds = Options.InitialRetrySeconds;
    Policy.MaxRetrySeconds = Options.MaxRetrySeconds; Policy.MaxResponseBytes = Options.MaxResponseBytes;
    Job = NewObject<UJWNU_JsonHttpJob>(this);
    Job->Initialize(EJWNU_HttpMethod::Post, Options.Endpoint, ApiKey, Body, {});
    Job->Configure(Policy, FJWNU_JsonHttpCompleted::CreateUObject(this, &UJWNU_SystemOneRequest::Receive));
    return true;
}

void UJWNU_SystemOneRequest::Pump()
{
    if (!IsActive()) { return; }
    if (Error.Code != EJWNU_SystemOneErrorCode::None) { Finish(); return; }
    if (!bStarted)
    {
        bStarted = true;
        if (!Job || !Job->Execute())
        { Error.Code = EJWNU_SystemOneErrorCode::Configuration; Error.Message = TEXT("Cannot start HTTP job."); Finish(); }
        return;
    }
    Job->Pump();
}

void UJWNU_SystemOneRequest::Receive(const FJWNU_JsonHttpResult& Response)
{
    if (!IsActive()) { return; }
    Error.HttpStatus = Response.StatusCode; Error.Attempts = Response.Attempts;
    Error.ResponseBody = Response.Body;
    switch (Response.Error)
    {
    case EJWNU_JsonHttpError::Network: Error.Code = EJWNU_SystemOneErrorCode::Network; Error.Message = TEXT("HTTP transport failed."); break;
    case EJWNU_JsonHttpError::Timeout: Error.Code = EJWNU_SystemOneErrorCode::Timeout; Error.Message = TEXT("Request deadline exceeded."); break;
    case EJWNU_JsonHttpError::Cancelled: Error.Code = EJWNU_SystemOneErrorCode::Cancelled; Error.Message = TEXT("Request cancelled."); break;
    case EJWNU_JsonHttpError::ResponseLimit: Error.Code = EJWNU_SystemOneErrorCode::ResponseLimit; Error.Message = TEXT("Response exceeded its byte limit."); break;
    default: break;
    }
    if (Error.Code == EJWNU_SystemOneErrorCode::None)
    {
        if (Response.StatusCode < 200 || Response.StatusCode >= 300)
        { Error.Code = EJWNU_SystemOneErrorCode::Http; Error.Message = TEXT("SystemOne returned an HTTP error; inspect HttpStatus and ResponseBody."); }
        else if (!FJWNU_SystemOneCodec::ParseResponse(Response.Body, SubmittedQuestions, Result, Error.Message))
        { Error.Code = EJWNU_SystemOneErrorCode::InvalidResponse; }
        else { Error = {}; Result.Attempts = Response.Attempts; }
    }
    Finish();
}

void UJWNU_SystemOneRequest::Finish()
{
    if (!IsActive()) { return; }
    TStrongObjectPtr<UJWNU_SystemOneRequest> KeepAlive(this);
    SetFinishedState(Error.Code == EJWNU_SystemOneErrorCode::None ? EJWNU_RequestState::Succeeded : (Error.Code == EJWNU_SystemOneErrorCode::Cancelled ? EJWNU_RequestState::Cancelled : EJWNU_RequestState::Failed)); Job = nullptr; SubmittedQuestions.Reset();
    if (Error.Code == EJWNU_SystemOneErrorCode::None)
    { OnCompletedNative.Broadcast(Result); OnCompleted.Broadcast(Result); }
    else { OnFailedNative.Broadcast(Error); OnFailed.Broadcast(Error); }
    BroadcastFinished();
}

void UJWNU_SystemOneRequest::CancelRequest()
{
    check(IsInGameThread());
    if (!IsActive()) { return; }
    if (Job && Job->IsRunning()) { Job->Cancel(); }
    else
    { Error = {}; Error.Code = EJWNU_SystemOneErrorCode::Cancelled; Error.Message = TEXT("Request cancelled before transmission."); Finish(); }
}
