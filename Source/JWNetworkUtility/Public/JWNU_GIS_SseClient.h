// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "JWNU_SseTypes.h"
#include "JWNU_HttpRequestJobHandle.h"

#include "JWNU_GIS_SseClient.generated.h"

class UJWNU_SseRequestBase;

/** SSE 요청의 GC 수명과 월드별 취소 및 기존 서비스 인증을 관리한다. */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_SseClient : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/** 월드에 속한 SSE 클라이언트를 반환하는 함수. */
	static UJWNU_GIS_SseClient* Get(const UObject* WorldContextObject);
private:
	friend class UJWNU_SseRequestBase;
	friend class UJWNU_SseRequest;
	friend class UJWNU_SseApiRequest;
	bool TrackRequest(UJWNU_SseRequestBase* Request);
	/** 내부 전송 핸들을 생성하고 직접 URL 요청을 예약하는 함수. */
	static UJWNU_HttpRequestJobHandle* SendSseRequest(const UObject* WorldContextObject,
		EJWNU_HttpMethod Method, const FString& URL, const FString& AuthToken, const FString& Body,
		const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks);
	/** 등록된 호스트와 JWT 갱신을 사용하는 SSE 요청 함수. */
	static UJWNU_HttpRequestJobHandle* CallSseApi_NoTemplate(const UObject* WorldContextObject,
		EJWNU_HttpMethod Method, EJWNU_ServiceType ServiceType, const FString& Endpoint, const FString& Body,
		const TMap<FString, FString>& QueryParams, const FJWNU_SseOptions& Options,
		const FJWNU_SseCallbacks& Callbacks, bool bRequiresAuth = true);
	UJWNU_HttpRequestJobHandle* MakeHandle(UWorld* World, const FJWNU_SseCallbacks& Callbacks);
	void StartRequest(UJWNU_HttpRequestJobHandle* Handle, EJWNU_HttpMethod Method, const FString& URL,
		const FString& Token, const FString& Body, const TMap<FString, FString>& Query,
		const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks);
	void StartService(UJWNU_HttpRequestJobHandle* Handle, EJWNU_HttpMethod Method, EJWNU_ServiceType ServiceType,
		const FString& Endpoint, const FString& Body, const TMap<FString, FString>& Query,
		const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks, bool bRequiresAuth, bool bCanRefresh);
	void RefreshThenStart(UJWNU_HttpRequestJobHandle* Handle, EJWNU_HttpMethod Method, EJWNU_ServiceType ServiceType,
		const FString& Endpoint, const FString& Body, const TMap<FString, FString>& Query,
		const FJWNU_SseOptions& Options, const FJWNU_SseCallbacks& Callbacks);
	bool Tick(float DeltaSeconds);
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	/** 공개 요청 객체를 종료까지 보관하는 필드. */
	UPROPERTY(Transient) TArray<TObjectPtr<UJWNU_SseRequestBase>> ActiveRequests;
	UPROPERTY() TArray<TObjectPtr<UJWNU_HttpRequestJobHandle>> ActiveHandles;
	TMap<TWeakObjectPtr<UJWNU_HttpRequestJobHandle>, TWeakObjectPtr<UWorld>> Worlds;
	TArray<TFunction<void()>> PendingStarts;
	FTSTicker::FDelegateHandle TickerHandle;
	FDelegateHandle CleanupHandle;
	bool bShuttingDown = false;
};
