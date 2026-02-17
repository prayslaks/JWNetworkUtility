// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.generated.h"


// ==================== JWNU HTTP Client Helpers & Jobs ====================


/**
 * HTTP 상태코드 열거형.
 * int32 HTTP 상태코드를 Blueprint에서 Switch/비교할 수 있도록 매핑한다.
 * 비HTTP 상태(네트워크 에러, 타임아웃, 파싱 에러)도 포함한다.
 */
UENUM(BlueprintType)
enum class EJWNU_HttpStatusCode : uint8
{
	// 기본값 / 비HTTP 상태
	None = 0				UMETA(DisplayName = "None"),
	NetworkError			UMETA(DisplayName = "Network Error"),
	Timeout					UMETA(DisplayName = "Timeout"),
	ParseError				UMETA(DisplayName = "Parse Error"),

	// 2XX 성공
	OK						UMETA(DisplayName = "200 OK"),
	Created					UMETA(DisplayName = "201 Created"),
	Accepted				UMETA(DisplayName = "202 Accepted"),
	NoContent				UMETA(DisplayName = "204 No Content"),

	// 3XX 리디렉션
	MovedPermanently		UMETA(DisplayName = "301 Moved Permanently"),
	Found					UMETA(DisplayName = "302 Found"),
	NotModified				UMETA(DisplayName = "304 Not Modified"),

	// 4XX 클라이언트 에러
	BadRequest				UMETA(DisplayName = "400 Bad Request"),
	Unauthorized			UMETA(DisplayName = "401 Unauthorized"),
	PaymentRequired			UMETA(DisplayName = "402 Payment Required"),
	Forbidden				UMETA(DisplayName = "403 Forbidden"),
	NotFound				UMETA(DisplayName = "404 Not Found"),
	MethodNotAllowed		UMETA(DisplayName = "405 Method Not Allowed"),
	NotAcceptable			UMETA(DisplayName = "406 Not Acceptable"),
	ProxyAuthRequired		UMETA(DisplayName = "407 Proxy Auth Required"),
	RequestTimeout			UMETA(DisplayName = "408 Request Timeout"),
	Conflict				UMETA(DisplayName = "409 Conflict"),
	Gone					UMETA(DisplayName = "410 Gone"),
	TooManyRequests			UMETA(DisplayName = "429 Too Many Requests"),

	// 5XX 서버 에러
	InternalServerError		UMETA(DisplayName = "500 Internal Server Error"),
	NotImplemented			UMETA(DisplayName = "501 Not Implemented"),
	BadGateway				UMETA(DisplayName = "502 Bad Gateway"),
	ServiceUnavailable		UMETA(DisplayName = "503 Service Unavailable"),
	GatewayTimeout			UMETA(DisplayName = "504 Gateway Timeout"),

	// 미등록 상태 코드
	UnknownError			UMETA(DisplayName = "Unknown Error"),
};

/**
 * int32 HTTP 상태 코드를 EJWNU_HttpStatusCode 열거형으로 변환한다.
 * 비HTTP 상태(NetworkError, Timeout, ParseError)는 호출자가 컨텍스트에 따라 직접 설정해야 한다.
 * @param StatusCode HTTP 상태 코드 (int32)
 * @return 대응하는 EJWNU_HttpStatusCode 열거값. 미등록 코드는 UnknownError 반환.
 */
inline EJWNU_HttpStatusCode JWNU_IntToHttpStatusCode(const int32 StatusCode)
{
	switch (StatusCode)
	{
	// 0: UE HTTP 모듈에서 응답 없음 (연결 실패 등)
	case 0:   return EJWNU_HttpStatusCode::NetworkError;

	// 2XX
	case 200: return EJWNU_HttpStatusCode::OK;
	case 201: return EJWNU_HttpStatusCode::Created;
	case 202: return EJWNU_HttpStatusCode::Accepted;
	case 204: return EJWNU_HttpStatusCode::NoContent;

	// 3XX
	case 301: return EJWNU_HttpStatusCode::MovedPermanently;
	case 302: return EJWNU_HttpStatusCode::Found;
	case 304: return EJWNU_HttpStatusCode::NotModified;

	// 4XX
	case 400: return EJWNU_HttpStatusCode::BadRequest;
	case 401: return EJWNU_HttpStatusCode::Unauthorized;
	case 402: return EJWNU_HttpStatusCode::PaymentRequired;
	case 403: return EJWNU_HttpStatusCode::Forbidden;
	case 404: return EJWNU_HttpStatusCode::NotFound;
	case 405: return EJWNU_HttpStatusCode::MethodNotAllowed;
	case 406: return EJWNU_HttpStatusCode::NotAcceptable;
	case 407: return EJWNU_HttpStatusCode::ProxyAuthRequired;
	case 408: return EJWNU_HttpStatusCode::RequestTimeout;
	case 409: return EJWNU_HttpStatusCode::Conflict;
	case 410: return EJWNU_HttpStatusCode::Gone;
	case 429: return EJWNU_HttpStatusCode::TooManyRequests;

	// 5XX
	case 500: return EJWNU_HttpStatusCode::InternalServerError;
	case 501: return EJWNU_HttpStatusCode::NotImplemented;
	case 502: return EJWNU_HttpStatusCode::BadGateway;
	case 503: return EJWNU_HttpStatusCode::ServiceUnavailable;
	case 504: return EJWNU_HttpStatusCode::GatewayTimeout;

	default:  return EJWNU_HttpStatusCode::UnknownError;
	}
}

/**
 * HTTP 메서드 열거형.
 */
UENUM(BlueprintType)
enum class EJWNU_HttpMethod : uint8
{
	Get,
	Post,
	Put,
	Delete,
};

/**
 * HTTP 리퀘스트 설정 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_RequestConfig
{
	GENERATED_BODY()

	/**
	 * 최대 재시도 횟수. (1이면 재시도 없이 1회만 시도)
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Config")
	int32 MaxRetries;

	/**
	 * 재시도 간 대기 시간 (초).
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Config")
	float RetryDelaySeconds;

	/**
	 * 요청 타임아웃 시간 (초).
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Config")
	float TimeoutSeconds;

	/**
	 * 5xx 서버 에러 시 재시도 여부.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Config")
	bool bRetryOn5XX;

	/**
	 * 타임아웃 시 재시도 여부.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Config")
	bool bRetryOnTimeout;

	/**
	 * 네트워크 에러 시 재시도 여부.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Config")
	bool bRetryOnNetworkError;

	/**
	 * 기본 생성자.
	 */
	FJWNU_RequestConfig()
	{
		MaxRetries = 3;
		RetryDelaySeconds = 1.0f;
		TimeoutSeconds = 30.0f;
		bRetryOn5XX = true;
		bRetryOnTimeout = true;
		bRetryOnNetworkError = true;
	}
};

/**
 * 401 발생 시 원래 요청을 재시도하기 위한 정보를 담는 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_PendingApiRequest
{
	GENERATED_BODY()

	EJWNU_ServiceType ServiceType;
	EJWNU_HttpMethod Method;
	FString URL;
	FString ContentBody;
	TMap<FString, FString> QueryParams;
};

/**
 * 토큰 리프레시 대기열에 적재되는 잡 구조체.
 * 리프레시 완료 시 OnTokenReady, 실패 시 OnTokenFailed가 호출된다.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_PendingJob
{
	GENERATED_BODY()
	
	FJWNU_PendingApiRequest RequestInfo;
	TFunction<void(const FString& /*NewAccessToken*/)> OnTokenReady;
	TFunction<void(const FString& /*ErrorCode*/, const FString& /*ErrorMessage*/)> OnTokenFailed;
};


// ==================== JWNU API Client Services ====================


/**
 * API 호출 시 서비스 타입 지정에 사용되는 열거형.
 */
UENUM(BlueprintType)
enum class EJWNU_ServiceType : uint8
{
	GameServer,
	AuthServer,
};

/**
 * 엑세스 토큰 값과 해당 토큰의 만료 시간을 저장하는 언리얼 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_AccessTokenContainer
{
	GENERATED_BODY()

	/**
	 * 인증 JWT 엑세스 토큰 값. 거의 대부분의 API에 Authorization으로 활용하며 서버에 의해 만료 시간이 관리된다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AccessToken;
	
	/**
	 * 엑세스 토큰의 만료 예정 시간을 유닉스 타임스탬프를 활용해, 클라이언트 단에서 판단할 수 있도록 한다.
	 * 단, 30초 정도의 버퍼 타임을 통해서 엑세스 토큰이 왠만하면 살아남을 수 있도록 관리한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 ExpiresAt;

	/**
	 * 기본 생성자.
	 */
	FJWNU_AccessTokenContainer()
	{
		AccessToken = TEXT("TRASH_TOKEN");
		ExpiresAt = -1;
	}
	
	/**
	 * 새로운 엑세스 토큰 컨테이너를 만들 때 사용하는 생성자.
	 * @param InAccessToken 엑세스 토큰
	 * @param InExpiresAt 만료 예정 시간
	 */
	FJWNU_AccessTokenContainer(const FString& InAccessToken, const int64 InExpiresAt)
	{
		AccessToken = InAccessToken;
		ExpiresAt = InExpiresAt;
	}
};

/**
 * 리프레시 토큰 값과 해당 토큰의 만료 시간을 저장하는 언리얼 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_RefreshTokenContainer
{
	GENERATED_BODY()
	
	/**
	 * 인증 Opaque 리프레시 토큰 값. 리프레시 API에 Authorization으로 활용하며 서버에 의해 만료 시간이 관리된다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RefreshToken;

	/**
	 * 리프레시 토큰의 만료 예정 시간을 유닉스 타임스탬프를 활용해, 클라이언트 단에서 판단할 수 있도록 한다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 ExpiresAt;
	
	/**
	 * 기본 생성자.
	 */
	FJWNU_RefreshTokenContainer()
	{
		RefreshToken = TEXT("TRASH_TOKEN");
		ExpiresAt = -1;
	}
	
	/**
	 * 새로운 리프레시 토큰 컨테이너를 만들 때 사용하는 생성자.
	 * @param InRefreshToken 리프레시 토큰
	 * @param InExpiresAt 만료 예정 시간
	 */
	FJWNU_RefreshTokenContainer(const FString& InRefreshToken, const int64 InExpiresAt)
	{
		RefreshToken = InRefreshToken;
		ExpiresAt = InExpiresAt;
	}
};

/**
 * 토큰 로드 시도의 결과를 지정하는 열거형. 블루프린트 지원에 활용된다.
 */
UENUM(BlueprintType)
enum class EJWNU_TokenGetResult : uint8
{
	Success = 0,
	Fail = 1,
	Empty = 2,	
};

/**
 * 토큰 저장 시도의 결과를 지정하는 열거형. 블루프린트 지원에 활용된다.
 */
UENUM(BlueprintType)
enum class EJWNU_TokenSetResult : uint8
{
	Success = 0,
	Fail = 1,
};

/**
 * 호스트 획득 시도의 결과를 지정하는 열거형. 블루프린트 지원에 활용된다.
 */
UENUM(BlueprintType)
enum class EJWNU_HostGetResult : uint8
{
	Fail = 0,
	Success = 1,
	Empty = 2,	
};

/**
 * 문자열로된 JSON 객체를 언리얼 구조체로 변환한 결과를 나타내는 구조체.
 */
UENUM(BlueprintType)
enum class EJWNU_ConvertJsonToStructResult : uint8
{
	Success = 0,
	Fail = 1,
	InvalidJSON = 2,
	NoMatch = 3,
};

/**
 * 언리얼 구조체를 JSON 문자열로 변환한 결과를 나타내는 열거형.
 */
UENUM(BlueprintType)
enum class EJWNU_ConvertStructToJsonResult : uint8
{
	Success = 0,
	Fail = 1,
	NoMatch = 2,
};


// ==================== JWNU Auth Widget Helper ====================


/**
 * 회원가입용 이메일 검증 열거형.
 */
UENUM(BlueprintType)
enum class EJWNU_RegisterEmailValidation : uint8
{
	Satisfied,
	Unsatisfied
};

/**
 * 회원가입용 1차 비밀번호 검증 열거형.
 */
UENUM(BlueprintType)
enum class EJWNU_RegisterPrimaryPasswordValidation : uint8
{
	Unsatisfied,
	Satisfied,
};

/**
 * 회원가입용 2차 비밀번호 검증 열거형.
 */
UENUM(BlueprintType)
enum class EJWNU_RegisterSecondaryPasswordValidation : uint8
{
	FirstPasswordEmpty,
	Unsatisfied,
	Satisfied,
};

/**
 * 로그인용 이메일 검증 열거형.
 */
UENUM(BlueprintType)
enum class EJWNU_LoginEmailValidation : uint8
{
	Satisfied,
	Unsatisfied
};

/**
 * 로그인용 비밀번호 검증 열거형.
 */
UENUM(BlueprintType)
enum class EJWNU_LoginPasswordValidation : uint8
{
	Satisfied,
	Unsatisfied
};


// ==================== JW Test Server API Request & Response ====================


/**
 * JW 커스텀 스타일 서버의 이메일 인증 API 요청 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_REQ_AuthRegisterSendCode
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Email;
};

/**
 * JW 커스텀 스타일 서버의 인증코드 제출 API 요청 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_REQ_AuthRegisterVerifyCode
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Email;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Code;
};

/**
 * JW 커스텀 스타일 서버의 회원가입 API 요청 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_REQ_AuthRegister
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Email;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Password;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Code;
};

/**
 * JW 커스텀 스타일 서버의 로그인 API 요청 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_REQ_AuthLogin
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Email;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Password;
};

/**
 * JW 커스텀 스타일 서버의 로그아웃 API 요청 구조체.
 */
USTRUCT(BLueprintType)
struct JWNETWORKUTILITY_API FJWNU_REQ_AuthLogout
{
	GENERATED_BODY()
	
	UPROPERTY(editAnywhere, BlueprintReadWrite)
	FString UserId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TargetServer;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RefreshToken;
};

/**
 * JW 커스텀 스타일 서버의 리프레시 API 요청 구조체
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_REQ_AuthRefresh
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString UserId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString TargetServer;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RefreshToken;
};

/**
 * JW 커스텀 스타일 서버의 데이터 API 요청 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_REQ_Data
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Data;
};

/**
 * JW 커스텀 스타일 서버의 기본 API 응답 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_RES_Base
{
	GENERATED_BODY()

	/**
	 * 비즈니스 로직의 성공 여부를 나타내는 불 필드. 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool Success;

	/**
	 * 네트워크, 파싱, 비즈니스 상태를 나타내는 커스텀 코드 문자열 필드.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Code;

	/**
	 * 네트워크, 파싱, 비즈니스 결과를 나타내는 메시지 문자열 필드.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Message;

	/**
	 * 기본 생성자
	 */
	FJWNU_RES_Base()
	{
		Success = false;
		Code = TEXT("UNKNOWN_ERROR");
		Message = TEXT("There is an Unknown Error");
	}
};

/**
 * JW 커스텀 스타일 서버의 토큰 리프레시 API 응답 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_RES_AuthRefresh : public FJWNU_RES_Base
{
	GENERATED_BODY()

	/**
	 * 새로 발급된 엑세스 토큰.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AccessToken;

	/**
	 * 새로 발급된 엑세스 토큰의 만료 시한을 나타내는 유닉스 타임스탬프.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 ExpiresAt;
	
	/**
	 * 새로 발급된 리프레시 토큰.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RefreshToken;

	/**
	 * 새로 발급된 리프레시 토큰의 만료 시한을 나타내는 유닉스 타임스탬프.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int64 RefreshTokenExpiresAt;

	/**
	 * 서버가 반환한 사용자 ID.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString UserId;

	/**
	 * 기본 생성자
	 */
	FJWNU_RES_AuthRefresh()
	{
		AccessToken = TEXT("");
		ExpiresAt = 0;
		RefreshToken = TEXT("");
		RefreshTokenExpiresAt = 0;
		UserId = TEXT("");
	}
};

/**
 * JW 커스텀 스타일 서버의 데이터 API 응답 구조체.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITY_API FJWNU_RES_Data : public FJWNU_RES_Base
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Data;
};
