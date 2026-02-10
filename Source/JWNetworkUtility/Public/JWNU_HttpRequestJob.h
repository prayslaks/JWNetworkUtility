// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNetworkUtilityDelegates.h"
#include "Interfaces/IHttpRequest.h"
#include "JWNU_HttpRequestJob.generated.h"

/** 
 * 클래스 전용 로그 카테고리 선언 
 */
JWNETWORKUTILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_HttpRequestJob, Log, All);

/**
 * 하나의 HTTP 요청을 Job 단위로 관리하는 클래스. 타임아웃, 재시도, 취소 등의 기능을 제공한다.
 */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_HttpRequestJob : public UObject
{
	GENERATED_BODY()

public:

#pragma region Fields for Configuration and Callback
	
	/** 
	 * 동작 설정 필드. 
	 */
	FJWNU_RequestConfig Config;
	
	/** 
	 * (상태 코드, 네트워크 상태, 리스폰스 바디)를 외부에 전달해주기 위한 델리게이트 필드.
	 */
	FOnHttpRequestJobCompletedDelegate OnHttpRequestJobComplete;

#pragma endregion

#pragma region Interface Methods for Initialize, Execute, Cancel Job Requests
	
	/**
	 * Job을 초기화하는 함수.
	 * @param InMethod HTTP 메서드
	 * @param InURL 요청 URL
	 * @param InAuthToken JWT 인증 토큰 (없으면 빈 문자열)
	 * @param InContentBody JSON 바디 (GET/DELETE는 빈 문자열)
	 * @param InConfig Job 설정
	 */
	UFUNCTION(Category="Job")
	void Initialize(EJWNU_HttpMethod InMethod, const FString& InURL, const FString& InAuthToken, const FString& InContentBody, const FJWNU_RequestConfig& InConfig);

	/**
	 * Job을 실행하는 함수.
	 */
	UFUNCTION(Category="Job")
	bool Execute();

	/**
	 * 진행 중인 Job을 취소하는 함수.
	 */
	UFUNCTION(Category="Job")
	void Cancel();

	/**
	 * Job이 현재 실행 중인지 반환하는 함수.
	 * @return 실행 중이면 true
	 */
	UFUNCTION(Category="Job|State")
	FORCEINLINE bool IsRunning() const { return bIsRunning; }

	/**
	 * Job이 취소됐는지 반환하는 함수.
	 * @return 취소됐다면 true
	 */
	UFUNCTION(Category="Job|State")
	FORCEINLINE bool IsCancelled() const {return bIsCancelled; }
	
#pragma endregion

private:
	
#pragma region Request Info Fields for Retry
	
	/** 
	 * HTTP 메서드 
	 */
	EJWNU_HttpMethod Method;

	/** 
	 * 요청 URL 
	 */
	FString URL;

	/** 
	 * JWT 인증 토큰 
	 */
	FString AuthToken;

	/** 
	 * JSON 바디 
	 */
	FString JsonBody;
	
#pragma endregion

#pragma region State Fields for Timeout-Retry-Cancel

	/** 
	 * 현재 시도 횟수를 나타내는 필드. 
	 */
	int32 CurrentAttempt = 0;

	/** 
	 * Job 실행 중 여부를 나타내는 필드.
	 */
	bool bIsRunning = false;

	/**
	 * Job 취소 여부를 나타내는 필드.
	 */
	bool bIsCancelled = false;
	
	/** 
	 * 현재 진행 중인 HTTP 요청, 중도 취소를 위한 포인터.
	 */
	TSharedPtr<IHttpRequest> CurrentRequest;

	/** 
	 * 재시도 딜레이 타이머 핸들 필드.
	 */
	FTimerHandle RetryTimerHandle;

	/** 
	 * 타임아웃 타이머 핸들 필드.
	 */
	FTimerHandle TimeoutTimerHandle;
	
#pragma endregion 
	
#pragma region Methods for Control Jobs

	/**
	 * 실제 HTTP 요청을 전송하는 내부 함수.
	 */
	void SendRequest();

	/**
	 * HTTP 응답 수신 시 호출되는 콜백 함수.
	 */
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bNetworkAvailable);

	/**
	 * 타임아웃 발생 시 호출되는 함수.
	 */
	void OnTimeout();

	/**
	 * 재시도를 스케줄링하는 함수.
	 */
	void ScheduleRetry();

	/**
	 * Job을 완료 처리하는 함수. 델리게이트를 호출하여 최종 결과를 외부에 전달하는 역할을 맡는다.
	 * @param StatusCode 상태 코드
	 * @param bNetworkAvailable 네트워크 성공 여부
	 * @param ResponseBody 최종 응답 바디
	 */
	void CompleteJob(const bool bNetworkAvailable, const int32 StatusCode, const FString& ResponseBody);

	/**
	 * 재시도 필요 여부를 판단하는 함수. 네트워크 실패와 서비스 실패 시 재시도 결정을 내린다.
	 * @param StatusCode HTTP 상태 코드
	 * @param bNetworkAvailable 네트워크 성공 여부
	 * @return 재시도가 필요하면 true
	 */
	bool ShouldRetry(const int32 StatusCode, const bool bNetworkAvailable) const;

	/**
	 * 타임아웃, 재시도 타이머를 정리하는 함수. 최종 성공, 실패, 취소 시 호출한다.
	 */
	void ClearAllTimers();
	
#pragma endregion

};
