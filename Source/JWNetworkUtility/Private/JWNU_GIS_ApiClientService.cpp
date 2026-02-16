// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_ApiClientService.h"
#include "JWNU_GIS_ApiHostProvider.h"

DEFINE_LOG_CATEGORY(LogJWNU_GIS_ApiClientService);

UJWNU_GIS_ApiClientService* UJWNU_GIS_ApiClientService::Get(const UObject* WorldContextObject)
{
	// 월드 컨텍스트 오브젝트 이상
	if (WorldContextObject == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("월드 컨텍스트 오브젝트 이상!"));
		return nullptr;
	}

	// 월드 획득
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("월드 획득 실패!"));
		return nullptr;
	}

	// 게임인스턴스 획득
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("게임 인스턴스 획득 실패!"));
		return nullptr;
	}
    
	// APIClientService 게임인스턴스 서브시스템 반환
	return GameInstance->GetSubsystem<UJWNU_GIS_ApiClientService>();
}

void UJWNU_GIS_ApiClientService::CallApi_NoTemplate(
	const UObject* WorldContextObject, 
	const EJWNU_HttpMethod InMethod,
	const EJWNU_ServiceType InServiceType, 
	const FString& InEndpoint, 
	const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams, 
	const FOnHttpResponseDelegate& OnHttpResponse,
	const FOnHttpRequestJobRetryDelegate& OnHttpRequestJobRetry, 
	const bool bRequiresAuth)
{
	// 객체 획득
	UJWNU_GIS_ApiClientService* Self = Get(WorldContextObject);
	if (Self == nullptr)
	{
		return;
	}

	// 호스트 프로바이더에서 호스트 획득
	FString ProvidedHost;
	if (const auto HostProvider = UJWNU_GIS_ApiHostProvider::Get(WorldContextObject))
	{
		if (HostProvider->GetHost(InServiceType, ProvidedHost) == false)
		{
			PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("호스트 획득 실패!"));
			const FString FakeResponseBody = TEXT("{\"success\": false, \"code\": \"HOST_NOT_FOUND\", \"message\": \"Failed to get host from provider\"}");
			OnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None, FakeResponseBody);
			return;
		}
	}

	// API URL 구축
	const FString ConstructedURL = ProvidedHost + InEndpoint;

	// 인증이 필요하지 않은 경우, 토큰 로직을 건너뛰고 바로 실행
	if (bRequiresAuth == false)
	{
		Self->CallApi_NoTemplate_Execution(InMethod, InServiceType, ConstructedURL, TEXT(""), InContentBody, InQueryParams, OnHttpResponse, OnHttpRequestJobRetry, false);
		return;
	}

	// 토큰 프로바이더에서 엑세스 토큰 획득
	FJWNU_AccessTokenContainer ProvidedAccessTokenContainer;
	if (const auto TokenProvider = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		if (TokenProvider->GetAccessTokenContainer(InServiceType, ProvidedAccessTokenContainer) == false)
		{
			PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("엑세스 토큰 획득 실패!"));
			const FString FakeResponseBody = TEXT("{\"success\": false, \"code\": \"TOKEN_NOT_FOUND\", \"message\": \"Failed to get access token from provider\"}");
			OnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None, FakeResponseBody);
			return;
		}
	}
	else
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("토큰 프로바이더 획득 실패!"));
		const FString FakeResponseBody = TEXT("{\"success\": false, \"code\": \"PROVIDER_NOT_FOUND\", \"message\": \"Failed to get token provider\"}");
		OnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None, FakeResponseBody);
		return;
	}

	// 토큰 만료 또는 리프레시 진행 중인 경우 잡 큐에 적재
	const int64 CurrentUnixTime = FDateTime::UtcNow().ToUnixTimestamp();
	const bool bTokenExpired = ProvidedAccessTokenContainer.ExpiresAt > 0 && CurrentUnixTime >= (ProvidedAccessTokenContainer.ExpiresAt - 30);
	if (bTokenExpired || Self->RefreshInProgressFlags.FindOrAdd(InServiceType))
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("엑세스 토큰 만료 또는 리프레시 진행 중, 잡 큐에 적재..."));
		JWNU_SCREEN_DEBUG(-1, 5.0f, FColor::Yellow, TEXT("[JWNU] Access Token Expired — Queuing refresh for %s"), *UEnum::GetValueAsString(InServiceType));
		FJWNU_PendingJob Job;
		Job.RequestInfo.ServiceType = InServiceType;
		Job.RequestInfo.Method = InMethod;
		Job.RequestInfo.URL = ConstructedURL;
		Job.RequestInfo.ContentBody = InContentBody;
		Job.RequestInfo.QueryParams = InQueryParams;
		Job.OnTokenReady = [Self, InMethod, InServiceType, ConstructedURL, InContentBody, InQueryParams, OnHttpResponse, OnHttpRequestJobRetry](const FString& NewAccessToken)
		{
			Self->CallApi_NoTemplate_Execution(InMethod, InServiceType, ConstructedURL, NewAccessToken, InContentBody, InQueryParams, OnHttpResponse, OnHttpRequestJobRetry, false);
		};
		Job.OnTokenFailed = [OnHttpResponse](const FString& ErrorCode, const FString& ErrorMessage)
		{
			const FString FakeResponseBody = FString::Printf(TEXT("{\"success\": false, \"code\": \"%s\", \"message\": \"%s\"}"), *ErrorCode, *ErrorMessage);
			OnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None, FakeResponseBody);
		};
		Self->RequestTokenRefresh(InServiceType, MoveTemp(Job));
		return;
	}

	Self->CallApi_NoTemplate_Execution(InMethod, InServiceType, ConstructedURL, ProvidedAccessTokenContainer.AccessToken, InContentBody, InQueryParams, OnHttpResponse, OnHttpRequestJobRetry, true);
}

void UJWNU_GIS_ApiClientService::CallApi_NoTemplate_Execution(
	const EJWNU_HttpMethod InMethod,
	const EJWNU_ServiceType InServiceType, 
	const FString& InURL, const FString& InAccessToken,
	const FString& InContentBody, 
	const TMap<FString, FString>& InQueryParams,
	const FOnHttpResponseDelegate& OnHttpResponse, 
	const FOnHttpRequestJobRetryDelegate& OnHttpRequestJobRetry,
	const bool bTryTokenRefreshing)
{
	if (bTryTokenRefreshing)
	{
		// 원래 요청 정보 저장 (401 발생 시 재시도용)
		FJWNU_PendingApiRequest PendingRequest;
		PendingRequest.ServiceType = InServiceType;
		PendingRequest.Method = InMethod;
		PendingRequest.URL = InURL;
		PendingRequest.ContentBody = InContentBody;
		PendingRequest.QueryParams = InQueryParams;		
		
		// 401 상태 코드를 처리할 수 있는 콜백
		const auto CallbackManage401 = FOnHttpRequestCompletedDelegate::CreateWeakLambda(this,
			[this, PendingRequest, OnHttpResponse, OnHttpRequestJobRetry](const int32 StatusCode, const FString& ResponseBody)
			{
				if (StatusCode == 401)
				{
					PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("401 토큰 만료 감지, 잡 큐에 적재 후 리프레시 시도..."));
					JWNU_SCREEN_DEBUG(-1, 5.0f, FColor::Orange, TEXT("[JWNU] 401 Unauthorized — Triggering token refresh for %s"), *UEnum::GetValueAsString(PendingRequest.ServiceType));
					FJWNU_PendingJob Job;
					Job.RequestInfo = PendingRequest;
					Job.OnTokenReady = [this, PendingRequest, OnHttpResponse, OnHttpRequestJobRetry](const FString& NewAccessToken)
					{
						CallApi_NoTemplate_Execution(PendingRequest.Method, PendingRequest.ServiceType, PendingRequest.URL, NewAccessToken, PendingRequest.ContentBody, PendingRequest.QueryParams, OnHttpResponse, OnHttpRequestJobRetry, false);
					};
					Job.OnTokenFailed = [OnHttpResponse](const FString& ErrorCode, const FString& ErrorMessage)
					{
						const FString FakeResponseBody = FString::Printf(TEXT("{\"success\": false, \"code\": \"%s\", \"message\": \"%s\"}"), *ErrorCode, *ErrorMessage);
						OnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None, FakeResponseBody);
					};
					RequestTokenRefresh(PendingRequest.ServiceType, MoveTemp(Job));
					return;
				}

				OnHttpResponse.ExecuteIfBound(JWNU_IntToHttpStatusCode(StatusCode), ResponseBody);
			});
		
		// Http 리퀘스트
		UJWNU_GIS_HttpClientHelper::SendReqeust_CustomResponse(GetWorld(), InMethod, InURL, InAccessToken, InContentBody, InQueryParams, CallbackManage401, OnHttpRequestJobRetry);
	}
	else
	{
		// 401 상태 코드를 처리하지 않는 콜백
		const auto CallbackNoManage401 = FOnHttpRequestCompletedDelegate::CreateWeakLambda(this,
			[this, OnHttpResponse](const int32 StatusCode, const FString& ResponseBody)
			{
				OnHttpResponse.ExecuteIfBound(JWNU_IntToHttpStatusCode(StatusCode), ResponseBody);
			});
	
		// Http 리퀘스트
		UJWNU_GIS_HttpClientHelper::SendReqeust_CustomResponse(GetWorld(), InMethod, InURL, InAccessToken, InContentBody, InQueryParams, CallbackNoManage401, OnHttpRequestJobRetry);
	}	
}

void UJWNU_GIS_ApiClientService::RequestTokenRefresh(const EJWNU_ServiceType InServiceType, FJWNU_PendingJob&& InJob)
{
	// 잡을 큐에 적재
	PendingJobQueues.FindOrAdd(InServiceType).Add(MoveTemp(InJob));

	// 이미 리프레시 진행 중이면 큐 적재만으로 종료
	bool& bRefreshing = RefreshInProgressFlags.FindOrAdd(InServiceType);
	if (bRefreshing)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("리프레시 이미 진행 중, 잡 큐에 적재만 수행"));
		return;
	}

	// 리프레시 시작
	bRefreshing = true;
	ExecuteTokenRefresh(InServiceType);
}

void UJWNU_GIS_ApiClientService::ExecuteTokenRefresh(EJWNU_ServiceType InServiceType)
{
	// IdentityProvider 획득
	const auto IdentityProvider = UJWNU_GIS_ApiIdentityProvider::Get(GetWorld());
	if (IdentityProvider == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("IdentityProvider 획득 실패!"));
		DrainPendingJobs_Failure(InServiceType, TEXT("IDENTITY_PROVIDER_NOT_FOUND"), TEXT("Failed to get identity provider"));
		return;
	}

	// 리프레시 토큰 컨테이너 획득
	FJWNU_RefreshTokenContainer RefreshTokenContainer;
	if (UJWNU_GIS_ApiIdentityProvider::GetRefreshTokenContainer(InServiceType, RefreshTokenContainer) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("리프레시 토큰 컨테이너 획득 실패!"));
		DrainPendingJobs_Failure(InServiceType, TEXT("REFRESH_TOKEN_NOT_FOUND"), TEXT("Failed to get refresh token container"));
		return;
	}

	// 리프레시 API 콜백
	const auto RefreshCallback = FOnHttpRequestCompletedDelegate::CreateWeakLambda(this,
		[this, InServiceType, IdentityProvider](const int32 StatusCode, const FString& ResponseBody)
		{
			// 언리얼 구조체 파싱 시도
			FJWNU_RES_AuthRefresh ResultData;
			if (FJsonObjectConverter::JsonObjectStringToUStruct(ResponseBody, &ResultData, 0, 0) == false)
			{
				DrainPendingJobs_Failure(InServiceType, TEXT("JSON_PARSE_ERROR"), TEXT("Failed to parse refresh response"));
				return;
			}

			// 파싱한 결과물 확인
			if (ResultData.Success == false || ResultData.AccessToken.IsEmpty())
			{
				PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("토큰 리프레시 실패: %s"), *ResultData.Message);
				DrainPendingJobs_Failure(InServiceType, TEXT("TOKEN_REFRESH_FAILED"), FString::Printf(TEXT("Token refresh failed: %s"), *ResultData.Message));
				return;
			}

			PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("토큰 리프레시 성공, 대기 중인 요청 일괄 처리..."));

			// 토큰 점검
			const FString NewAccessToken = ResultData.AccessToken;
			const FString ExpireDateTime = FDateTime::FromUnixTimestamp(ResultData.ExpiresAt).ToString();
			const FString NewRefreshToken = ResultData.RefreshToken;
			const FString RefreshExpireDateTime = FDateTime::FromUnixTimestamp(ResultData.RefreshTokenExpiresAt).ToString();
			PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("AccessToken : %s\nExpiresAt : %s\nRefreshToken : %s\nRefreshTokenExpiresAt : %s"), *NewAccessToken, *ExpireDateTime, *NewRefreshToken, *RefreshExpireDateTime);

			// 인증 서버로부터 엑세스 토큰과 리프레시 토큰 컨테이너 갱신
			IdentityProvider->SetAccessTokenContainer(InServiceType, { NewAccessToken, ResultData.ExpiresAt });
			IdentityProvider->SetRefreshTokenContainer(InServiceType, { NewRefreshToken, ResultData.RefreshTokenExpiresAt });

			// 리프레시 응답에서 UserId 저장
			if (!ResultData.UserId.IsEmpty())
			{
				IdentityProvider->SetUserId(ResultData.UserId);
			}

			// 대기 중인 잡 일괄 처리
			DrainPendingJobs_Success(InServiceType, NewAccessToken);
		});

	// 리프레시 API 호출
	const FString RefreshURL = BuildRefreshTokenURL();
	const FString TargetServer = UEnum::GetValueAsString(InServiceType);
	const FString CurrentUserId = IdentityProvider->GetUserId();
	const FString RefreshBody = FString::Printf(
		TEXT("{\"userId\": \"%s\", \"targetServer\": \"%s\", \"refreshToken\": \"%s\"}"),
		*CurrentUserId, *TargetServer, *RefreshTokenContainer.RefreshToken);
	JWNU_SCREEN_DEBUG(-1, 5.0f, FColor::Cyan, TEXT("[JWNU] Calling Refresh API → %s"), *RefreshURL);
	UJWNU_GIS_HttpClientHelper::SendReqeust_CustomResponse(GetWorld(), EJWNU_HttpMethod::Post, RefreshURL, TEXT(""), RefreshBody, {}, RefreshCallback);
}

void UJWNU_GIS_ApiClientService::DrainPendingJobs_Success(const EJWNU_ServiceType InServiceType, const FString& NewAccessToken)
{
	// 플래그 해제
	RefreshInProgressFlags.FindOrAdd(InServiceType) = false;

	// 큐에서 잡을 꺼내어 일괄 처리
	TArray<FJWNU_PendingJob> Jobs;
	if (auto* Queue = PendingJobQueues.Find(InServiceType))
	{
		Jobs = MoveTemp(*Queue);
	}

	PRINT_LOG(LogJWNU_GIS_ApiClientService, Display, TEXT("리프레시 성공, 대기 잡 %d개 처리 시작"), Jobs.Num());
	JWNU_SCREEN_DEBUG(-1, 5.0f, FColor::Green, TEXT("[JWNU] Token Refresh SUCCESS — Draining %d pending jobs"), Jobs.Num());
	for (auto& Job : Jobs)
	{
		if (Job.OnTokenReady)
		{
			Job.OnTokenReady(NewAccessToken);
		}
	}
}

void UJWNU_GIS_ApiClientService::DrainPendingJobs_Failure(const EJWNU_ServiceType InServiceType, const FString& ErrorCode, const FString& ErrorMessage)
{
	// 플래그 해제
	RefreshInProgressFlags.FindOrAdd(InServiceType) = false;

	// 큐에서 잡을 꺼내어 일괄 실패 처리
	TArray<FJWNU_PendingJob> Jobs;
	if (auto* Queue = PendingJobQueues.Find(InServiceType))
	{
		Jobs = MoveTemp(*Queue);
	}

	PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("리프레시 실패, 대기 잡 %d개 에러 처리 (Code: %s)"), Jobs.Num(), *ErrorCode);
	JWNU_SCREEN_DEBUG(-1, 7.0f, FColor::Red, TEXT("[JWNU] Token Refresh FAILED — %s: %s (%d jobs drained)"), *ErrorCode, *ErrorMessage, Jobs.Num());
	for (auto& Job : Jobs)
	{
		if (Job.OnTokenFailed)
		{
			Job.OnTokenFailed(ErrorCode, ErrorMessage);
		}
	}
}

FString UJWNU_GIS_ApiClientService::BuildRefreshTokenURL() const
{
	FString Host;
	if (UJWNU_GIS_ApiHostProvider::Get(GetWorld())->GetHost(EJWNU_ServiceType::AuthServer, Host))
	{
		return FString::Printf(TEXT("%s/auth/refresh"), *Host);
	}

	PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("ApiHostProvider 획득 실패, 기본 URL 반환"));
	return TEXT("http://localhost:5000/auth/refresh");
}
