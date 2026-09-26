// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNU_HttpRequestJob.h"
#include "Engine/Engine.h"
#include "JWNU_HttpRequestJobHandle.generated.h"

/**
 * HTTP 요청의 논리적 수명을 추적하는 핸들 클래스.
 * 401 토큰 리프레시 시 내부 Job이 교체되더라도 동일한 Handle을 통해 요청을 제어할 수 있다.
 */
UCLASS(NotBlueprintType, NotBlueprintable)
class JWNETWORKUTILITY_API UJWNU_HttpRequestJobHandle : public UObject
{
	GENERATED_BODY()

public:
	/** 인증 갱신 대기를 포함해 요청이 활성 상태인지 반환하는 함수. */
	bool IsActive() const;

	/**
	 * 진행 중인 요청을 취소하는 함수.
	 */
	void Cancel();


	/**
	 * 요청이 취소됐는지 반환하는 함수.
	 * @return 취소됐다면 true
	 */
	bool IsCancelled() const;

	/**
	 * 새 Job을 바인딩하는 내부 함수. 초기 생성 또는 401 리프레시 후 새 Job 바인딩에 사용한다.
	 * @param InJob 바인딩할 Job
	 */
	void BindJob(UJWNU_HttpRequestJob* InJob);

	/**
	 * 401 리프레시 대기 상태로 전환하는 내부 함수. IsActive()이 true를 유지하도록 한다.
	 */
	void MarkWaitingForRefresh();

	/**
	 * 401 리프레시 대기 상태를 해제하는 내부 함수.
	 */
	void ClearWaitingForRefresh();

	/** 스트림 관리자가 현재 Job을 조회하는 함수. */
	UJWNU_HttpRequestJob* GetJob() const { return CurrentJob; }
	/** Job 생성 또는 인증 갱신 대기 중 SSE 취소 알림을 지정하는 함수. */
	void SetSsePendingCancel(const FSimpleDelegate& Callback) { SsePendingCancel = Callback; }

private:

	/**
	 * 현재 바인딩된 HTTP 요청 Job.
	 */
	UPROPERTY()
	TObjectPtr<UJWNU_HttpRequestJob> CurrentJob;

	/**
	 * 취소 여부를 나타내는 플래그.
	 */
	bool bIsCancelled = false;

	/**
	 * 401 리프레시 대기 중 여부를 나타내는 플래그. IsActive()이 true를 유지하도록 한다.
	 */
	bool bIsWaitingForRefresh = false;
	FSimpleDelegate SsePendingCancel;
};
