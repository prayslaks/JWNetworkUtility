// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_SseClient.h"
#include "JWNU_GIS_ApiClientService.h"
#include "JWNU_SseRequestJob.h"
#include "JWNU_SseRequest.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformHttp.h"

void UJWNU_GIS_SseClient::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UJWNU_GIS_ApiClientService>();
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UJWNU_GIS_SseClient::Tick));
	CleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UJWNU_GIS_SseClient::OnWorldCleanup);
}

void UJWNU_GIS_SseClient::Deinitialize()
{
	bShuttingDown = true;
	FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	FWorldDelegates::OnWorldCleanup.Remove(CleanupHandle);
	TArray<TStrongObjectPtr<UJWNU_SseRequestBase>> Requests;
	for (auto Request : ActiveRequests) { Requests.Emplace(Request); }
	for (const auto& Request : Requests) { Request->Cancel(); }
	TArray<TStrongObjectPtr<UJWNU_HttpRequestJobHandle>> Snapshot;
	for (auto Handle : ActiveHandles) { Snapshot.Emplace(Handle); }
	for (const auto& Handle : Snapshot) { if (Handle->IsActive()) { Handle->Cancel(); } }
	ActiveRequests.Reset();
	PendingStarts.Reset(); ActiveHandles.Reset(); Worlds.Reset();
	Super::Deinitialize();
}

UJWNU_GIS_SseClient* UJWNU_GIS_SseClient::Get(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UJWNU_GIS_SseClient>() : nullptr;
}

UJWNU_HttpRequestJobHandle* UJWNU_GIS_SseClient::MakeHandle(UWorld* World, const FJWNU_SseCallbacks& Callbacks)
{
	if (bShuttingDown || !World || World->bIsTearingDown) { return nullptr; }
	auto* Handle = NewObject<UJWNU_HttpRequestJobHandle>(this);
	Handle->MarkWaitingForRefresh();
	Handle->SetSsePendingCancel(FSimpleDelegate::CreateLambda([Callback = Callbacks.OnCancelled]() { Callback.ExecuteIfBound(); }));
	ActiveHandles.Add(Handle); Worlds.Add(Handle, World);
	return Handle;
}

UJWNU_HttpRequestJobHandle* UJWNU_GIS_SseClient::SendSseRequest(const UObject* WorldContextObject,
	EJWNU_HttpMethod Method, const FString& URL, const FString& AuthToken, const FString& Body,
	const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks)
{
	check(IsInGameThread());
	auto* Self = Get(WorldContextObject);
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	auto* Handle = Self ? Self->MakeHandle(World, Callbacks) : nullptr;
	if (!Handle) { FJWNU_SseResponse Error; Error.Error = EJWNU_SseError::InvalidRequest; Error.Message = TEXT("SSE requires an active GameInstance world"); Callbacks.OnError.ExecuteIfBound(Error); return nullptr; }
	Self->PendingStarts.Add([Self, Handle, Method, URL, AuthToken, Body, QueryParams, Options, Callbacks]()
	{
		Self->StartRequest(Handle, Method, URL, AuthToken, Body, QueryParams, Options, Callbacks);
	});
	return Handle;
}

UJWNU_HttpRequestJobHandle* UJWNU_GIS_SseClient::CallSseApi_NoTemplate(const UObject* WorldContextObject,
	EJWNU_HttpMethod Method, EJWNU_ServiceType ServiceType, const FString& Endpoint, const FString& Body,
	const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks, bool bRequiresAuth)
{
	check(IsInGameThread());
	auto* Self = Get(WorldContextObject);
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	auto* Handle = Self ? Self->MakeHandle(World, Callbacks) : nullptr;
	if (!Handle) { FJWNU_SseResponse Error; Error.Error = EJWNU_SseError::InvalidRequest; Error.Message = TEXT("SSE requires an active GameInstance world"); Callbacks.OnError.ExecuteIfBound(Error); return nullptr; }
	Self->PendingStarts.Add([Self, Handle, Method, ServiceType, Endpoint, Body, QueryParams, Options, Callbacks, bRequiresAuth]()
	{
		Self->StartService(Handle, Method, ServiceType, Endpoint, Body, QueryParams, Options, Callbacks, bRequiresAuth, true);
	});
	return Handle;
}

void UJWNU_GIS_SseClient::StartRequest(UJWNU_HttpRequestJobHandle* Handle, EJWNU_HttpMethod Method,
	const FString& URL, const FString& Token, const FString& Body, const TMap<FString, FString>& Query,
	const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks)
{
	if (Handle->IsCancelled() || bShuttingDown) { return; }
	FString FinalURL = URL;
	for (const auto& Pair : Query)
	{
		FinalURL += FinalURL.Contains(TEXT("?")) ? TEXT("&") : TEXT("?");
		FinalURL += FGenericPlatformHttp::UrlEncode(Pair.Key) + TEXT("=") + FGenericPlatformHttp::UrlEncode(Pair.Value);
	}
	auto* Job = NewObject<UJWNU_SseRequestJob>(this);
	Job->Initialize(Method, FinalURL, Token, Body, FJWNU_RequestConfig());
	Job->ConfigureSse(Options, Callbacks);
	Handle->BindJob(Job); Handle->ClearWaitingForRefresh();
	Job->Execute();
}

void UJWNU_GIS_SseClient::StartService(UJWNU_HttpRequestJobHandle* Handle, EJWNU_HttpMethod Method,
	EJWNU_ServiceType ServiceType, const FString& Endpoint, const FString& Body, const TMap<FString, FString>& Query,
	const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks, bool bRequiresAuth, bool bCanRefresh)
{
	if (Handle->IsCancelled() || bShuttingDown) { return; }
	FString Host;
	const auto* HostProvider = GetGameInstance()->GetSubsystem<UJWNU_GIS_ApiHostProvider>();
	const auto* Identity = GetGameInstance()->GetSubsystem<UJWNU_GIS_ApiIdentityProvider>();
	auto* Service = GetGameInstance()->GetSubsystem<UJWNU_GIS_ApiClientService>();
	FJWNU_AccessTokenContainer Token;
	if (!HostProvider || !HostProvider->GetHost(ServiceType, Host) || !Service
		|| (bRequiresAuth && (!Identity || !Identity->GetAccessTokenContainer(ServiceType, Token))))
	{
		Handle->ClearWaitingForRefresh();
		FJWNU_SseResponse Error; Error.Error = EJWNU_SseError::Authentication; Error.Message = TEXT("Service host or authentication token is unavailable");
		Callbacks.OnError.ExecuteIfBound(Error); return;
	}
	if (bRequiresAuth && bCanRefresh && ((Token.ExpiresAt > 0 && FDateTime::UtcNow().ToUnixTimestamp() >= Token.ExpiresAt - 30)
		|| Service->RefreshInProgressFlags.FindRef(ServiceType)))
	{
		RefreshThenStart(Handle, Method, ServiceType, Endpoint, Body, Query, Options, Callbacks); return;
	}
	FJWNU_SseCallbacks Wrapped = Callbacks;
	Wrapped.OnError = FJWNU_OnSseResponse::CreateWeakLambda(this,
		[this, Handle, Method, ServiceType, Endpoint, Body, Query, Options, Callbacks, bRequiresAuth, bCanRefresh](const FJWNU_SseResponse& Error)
		{
			if (Handle->IsCancelled()) { return; }
			if (bRequiresAuth && bCanRefresh && Error.Error == EJWNU_SseError::Http && Error.StatusCode == 401)
			{
				RefreshThenStart(Handle, Method, ServiceType, Endpoint, Body, Query, Options, Callbacks);
			}
			else { Callbacks.OnError.ExecuteIfBound(Error); }
		});
	FJWNU_SseOptions ServiceOptions = Options;
	// 서비스 호출에서는 등록된 JWT만 인증에 사용한다. 임의 키는 직접 URL 경로로 전달한다.
	if (bRequiresAuth)
	{
		for (auto It = ServiceOptions.Headers.CreateIterator(); It; ++It)
		{
			if (It.Key().Equals(TEXT("Authorization"), ESearchCase::IgnoreCase)) { It.RemoveCurrent(); }
		}
	}
	StartRequest(Handle, Method, Host + Endpoint, bRequiresAuth ? Token.AccessToken : FString(), Body, Query, ServiceOptions, Wrapped);
}

void UJWNU_GIS_SseClient::RefreshThenStart(UJWNU_HttpRequestJobHandle* Handle, EJWNU_HttpMethod Method,
	EJWNU_ServiceType ServiceType, const FString& Endpoint, const FString& Body, const TMap<FString, FString>& Query,
	const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks)
{
	Handle->MarkWaitingForRefresh();
	const TWeakObjectPtr<UJWNU_GIS_SseClient> WeakSelf(this);
	const TWeakObjectPtr<UJWNU_HttpRequestJobHandle> WeakHandle(Handle);
	FJWNU_PendingJob Pending;
	Pending.OnTokenReady = [WeakSelf, WeakHandle, Method, ServiceType, Endpoint, Body, Query, Options, Callbacks](const FString&)
	{
		if (WeakSelf.IsValid() && WeakHandle.IsValid() && !WeakHandle->IsCancelled())
		{
			WeakSelf->StartService(WeakHandle.Get(), Method, ServiceType, Endpoint, Body, Query, Options, Callbacks, true, false);
		}
	};
	Pending.OnTokenFailed = [WeakHandle, Callbacks](const FString& Code, const FString& Message)
	{
		if (!WeakHandle.IsValid() || WeakHandle->IsCancelled()) { return; }
		WeakHandle->ClearWaitingForRefresh();
		FJWNU_SseResponse Error; Error.Error = EJWNU_SseError::Authentication; Error.Message = Code + TEXT(": ") + Message;
		Callbacks.OnError.ExecuteIfBound(Error);
	};
	GetGameInstance()->GetSubsystem<UJWNU_GIS_ApiClientService>()->RequestTokenRefresh(ServiceType, MoveTemp(Pending));
}

bool UJWNU_GIS_SseClient::Tick(float DeltaSeconds)
{
	TArray<TStrongObjectPtr<UJWNU_SseRequestBase>> Requests;
	for (auto Request : ActiveRequests) { Requests.Emplace(Request); }
	for (const auto& Request : Requests)
	{ if (!Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { Request->Cancel(); } }
	TArray<TStrongObjectPtr<UJWNU_HttpRequestJobHandle>> Snapshot;
	for (auto Handle : ActiveHandles) { Snapshot.Emplace(Handle); }
	for (const auto& Handle : Snapshot)
	{
		const auto* World = Worlds.Find(Handle.Get());
		if (!World || !World->IsValid() || World->Get()->bIsTearingDown) { Handle->Cancel(); }
	}
	auto Starts = MoveTemp(PendingStarts);
	for (auto& Start : Starts) { Start(); }
	for (const auto& Handle : Snapshot)
	{
		// 콜백에서 인증 갱신·월드 종료·GC가 발생해도 현재 Pump의 Job을 보호한다.
		TStrongObjectPtr<UJWNU_SseRequestJob> Job(Cast<UJWNU_SseRequestJob>(Handle->GetJob()));
		if (Job.IsValid()) { Job->Pump(); }
	}
	for (int32 Index = ActiveHandles.Num() - 1; Index >= 0; --Index)
	{
		if (!ActiveHandles[Index]->IsActive()) { Worlds.Remove(ActiveHandles[Index]); ActiveHandles.RemoveAt(Index); }
	}
	ActiveRequests.RemoveAll([](const auto& Request) { return !Request->IsActive(); });
	return true;
}

void UJWNU_GIS_SseClient::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	TArray<TStrongObjectPtr<UJWNU_SseRequestBase>> Snapshot;
	for (auto Request : ActiveRequests) { Snapshot.Emplace(Request); }
	for (const auto& Request : Snapshot)
	{
		if (Request->OwnerWorld.Get() == World) { Request->Cancel(); }
	}
}

bool UJWNU_GIS_SseClient::TrackRequest(UJWNU_SseRequestBase* Request)
{
	if (bShuttingDown || !Request->OwnerWorld.IsValid() || Request->OwnerWorld->bIsTearingDown) { return false; }
	ActiveRequests.AddUnique(Request); return true;
}
