// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_HttpRequestJob.h"
#include "HttpModule.h"
#include "JWNetworkUtility.h"
#include "Interfaces/IHttpResponse.h"
#include "TimerManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogJWNU_HttpRequestJob);

void UJWNU_HttpRequestJob::Initialize(const EJWNU_HttpMethod InMethod, const FString& InURL, const FString& InAuthToken, const FString& InContentBody, const FJWNU_RequestConfig& InConfig)
{
	Method = InMethod;
	URL = InURL;
	AuthToken = InAuthToken;
	JsonBody = InContentBody;
	Config = InConfig;

	// 상태 초기화
	CurrentAttempt = 0;
	bIsRunning = false;
	bIsCancelled = false;
}

bool UJWNU_HttpRequestJob::Execute()
{
	// 이미 실행 중인 경우 무시
	if (bIsRunning)
	{
		PRINT_LOG(LogJWNU_HttpRequestJob, Warning, TEXT("Job이 이미 실행 중입니다."));
		return false;
	}

	// 상태 초기화
	bIsRunning = true;
	bIsCancelled = false;
	CurrentAttempt = 0;

	// 첫 번째 요청 전송
	SendRequest();
	return true;
}

void UJWNU_HttpRequestJob::Cancel()
{
	// 이미 취소되었거나 실행 중이 아닌 경우 무시
	if (bIsCancelled || !bIsRunning)
	{
		return;
	}

	bIsCancelled = true;

	// 모든 타이머 정리
	ClearAllTimers();

	// 진행 중인 HTTP 요청 취소
	if (CurrentRequest.IsValid())
	{
		CurrentRequest->CancelRequest();
		CurrentRequest.Reset();
	}

	bIsRunning = false;

	PRINT_LOG(LogJWNU_HttpRequestJob, Display, TEXT("Job이 취소되었습니다. (총 시도 횟수: %d)"), CurrentAttempt);
}

void UJWNU_HttpRequestJob::SendRequest()
{
	// 취소된 경우 중단
	if (bIsCancelled)
	{
		return;
	}

	CurrentAttempt++;
	PRINT_LOG(LogJWNU_HttpRequestJob, Display, TEXT("HTTP 요청 시도 %d/%d: %s"), CurrentAttempt, Config.MaxRetries, *URL);

	// HTTP 요청 객체 생성
	CurrentRequest = FHttpModule::Get().CreateRequest();

	// HTTP 메서드 문자열 변환
	FString MethodString;
	switch (Method)
	{
	case EJWNU_HttpMethod::Get:
		MethodString = TEXT("GET");
		break;
	case EJWNU_HttpMethod::Post:
		MethodString = TEXT("POST");
		break;
	case EJWNU_HttpMethod::Put:
		MethodString = TEXT("PUT");
		break;
	case EJWNU_HttpMethod::Delete:
		MethodString = TEXT("DELETE");
		break;
	}

	// 요청 설정
	CurrentRequest->SetVerb(MethodString);
	CurrentRequest->SetURL(URL);
	CurrentRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	// JWT 인증 토큰 설정
	if (!AuthToken.IsEmpty())
	{
		CurrentRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
	}

	// JSON 바디 설정 (POST/PUT)
	if (!JsonBody.IsEmpty())
	{
		CurrentRequest->SetContentAsString(JsonBody);
	}

	// 응답 콜백 바인딩
	CurrentRequest->OnProcessRequestComplete().BindUObject(this, &UJWNU_HttpRequestJob::OnResponseReceived);

	// 타임아웃 타이머 설정
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TimeoutTimerHandle,
			this,
			&UJWNU_HttpRequestJob::OnTimeout,
			Config.TimeoutSeconds,
			false
		);
	}

	// 요청 실행
	CurrentRequest->ProcessRequest();
}

void UJWNU_HttpRequestJob::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, const bool bNetworkAvailable)
{
	// 취소된 경우 무시
	if (bIsCancelled)
	{
		return;
	}

	// 타임아웃 타이머 해제
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
	}
	else
	{
		PRINT_LOG(LogJWNU_HttpRequestJob, Error, TEXT("Failed to GetWorld!"));
	}

	// 상태 코드 획득
	const int32 StatusCode = Response.IsValid() ? Response->GetResponseCode() : 0;
	PRINT_LOG(LogJWNU_HttpRequestJob, Display, TEXT("HTTP 응답 수신 - 상태 코드: %d, 네트워크 성공: %s"), StatusCode, bNetworkAvailable ? TEXT("true") : TEXT("false"));

	// 재시도 필요 여부 판단
	if (ShouldRetry(StatusCode, bNetworkAvailable) && CurrentAttempt < Config.MaxRetries)
	{
		ScheduleRetry();
		return;
	}

	// 리스폰스 바디 획득
	const FString ResponseBody = Response.IsValid() ? Response->GetContentAsString() : TEXT("");

	// Job 최종 처리 단계
	CompleteJob(bNetworkAvailable, StatusCode, ResponseBody);
}

void UJWNU_HttpRequestJob::OnTimeout()
{
	// 취소된 경우 무시
	if (bIsCancelled)
	{
		return;
	}

	PRINT_LOG(LogJWNU_HttpRequestJob, Warning, TEXT("HTTP 요청 타임아웃 (시도 %d/%d)"), CurrentAttempt, Config.MaxRetries);

	// 현재 요청 취소
	if (CurrentRequest.IsValid())
	{
		CurrentRequest->CancelRequest();
		CurrentRequest.Reset();
	}

	// 타임아웃 시 재시도 여부 판단
	if (Config.bRetryOnTimeout && CurrentAttempt < Config.MaxRetries)
	{
		ScheduleRetry();
		return;
	}

	// 타임아웃 실패에 대한 리스폰스 바디 생성
	const FString TimeoutResponse = TEXT("{\"message\": \"This is Message from JWNetworkUtility Plugin. Not from Unreal Engine Http Module. Http Request timed out\"}");
	
	// Job 최종 처리 단계
	CompleteJob(false, 408, TimeoutResponse);
}

void UJWNU_HttpRequestJob::ScheduleRetry()
{
	// 취소된 경우 무시
	if (bIsCancelled)
	{
		return;
	}

	PRINT_LOG(LogJWNU_HttpRequestJob, Display, TEXT("%.1f초 후 재시도 예정 (다음 시도: %d/%d)"), Config.RetryDelaySeconds, CurrentAttempt + 1, Config.MaxRetries);

	// 재시도 이벤트 브로드캐스트
	OnHttpRequestJobRetry.ExecuteIfBound(CurrentAttempt + 1);

	// 딜레이 후 재시도
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RetryTimerHandle,
			this,
			&UJWNU_HttpRequestJob::SendRequest,
			Config.RetryDelaySeconds,
			false
		);
	}
}

void UJWNU_HttpRequestJob::CompleteJob(const bool bNetworkAvailable, const int32 StatusCode, const FString& ResponseBody)
{
	// 상태 정리
	bIsRunning = false;
	CurrentRequest.Reset();
	ClearAllTimers();
	
	PRINT_LOG(LogJWNU_HttpRequestJob, Display, TEXT("Job 완료 - 서비스 성공: %s, 총 시도 횟수: %d"), bNetworkAvailable ? TEXT("true") : TEXT("false"), CurrentAttempt);
	
	OnHttpRequestJobComplete.ExecuteIfBound(bNetworkAvailable, StatusCode, ResponseBody);
}

bool UJWNU_HttpRequestJob::ShouldRetry(const int32 StatusCode, const bool bNetworkAvailable) const
{
	// 네트워크 실패
	if (bNetworkAvailable == false && Config.bRetryOnNetworkError)
	{
		PRINT_LOG(LogJWNU_HttpRequestJob, Warning, TEXT("네트워크 에러로 재시도 필요..."));
		return true;
	}

	// 5XX 서버 에러
	if (Config.bRetryOn5XX && StatusCode >= 500 && StatusCode < 600)
	{
		PRINT_LOG(LogJWNU_HttpRequestJob, Warning, TEXT("서버 에러(%d)로 재시도 필요..."), StatusCode);
		return true;
	}

	return false;
}

void UJWNU_HttpRequestJob::ClearAllTimers()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryTimerHandle);
		World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
		PRINT_LOG(LogJWNU_HttpRequestJob, Display, TEXT("타이머 정리 완료"));
	}
}
