// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNU_HttpRequestJob.h"
#include "JWNU_HttpRequestJobHandle.generated.h"

/**
 * HTTP 요청의 논리적 수명을 추적하는 핸들 클래스.
 * 401 토큰 리프레시 시 내부 Job이 교체되더라도 동일한 Handle을 통해 요청을 제어할 수 있다.
 */
UCLASS(BlueprintType)
class JWNETWORKUTILITY_API UJWNU_HttpRequestJobHandle : public UObject
{
	GENERATED_BODY()

public:

	/**
	 * 진행 중인 요청을 취소하는 함수.
	 */
	UFUNCTION(BlueprintCallable, Category="JWNU|Job Control")
	void Cancel();

	/**
	 * 요청이 현재 실행 중인지 반환하는 함수.
	 * Job이 실행 중이거나 401 리프레시 대기 중이면 true를 반환한다.
	 * @return 실행 중이면 true
	 */
	UFUNCTION(BlueprintPure, Category="JWNU|Job Control")
	bool IsRunning() const;

	/**
	 * 요청이 취소됐는지 반환하는 함수.
	 * @return 취소됐다면 true
	 */
	UFUNCTION(BlueprintPure, Category="JWNU|Job Control")
	bool IsCancelled() const;

	/**
	 * 새 Job을 바인딩하는 내부 함수. 초기 생성 또는 401 리프레시 후 새 Job 바인딩에 사용한다.
	 * @param InJob 바인딩할 Job
	 */
	void BindJob(UJWNU_HttpRequestJob* InJob);

	/**
	 * 401 리프레시 대기 상태로 전환하는 내부 함수. IsRunning()이 true를 유지하도록 한다.
	 */
	void MarkWaitingForRefresh();

	/**
	 * 401 리프레시 대기 상태를 해제하는 내부 함수.
	 */
	void ClearWaitingForRefresh();

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
	 * 401 리프레시 대기 중 여부를 나타내는 플래그. IsRunning()이 true를 유지하도록 한다.
	 */
	bool bIsWaitingForRefresh = false;
};
