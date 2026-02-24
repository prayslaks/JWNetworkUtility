# JWNetworkUtility Plugin

Unreal Engine 5.6용 독립 플러그인으로, JWT 인증, 자동 토큰 갱신, 재시도/타임아웃 처리, Blueprint 지원을 갖춘 계층형 HTTP API 클라이언트 시스템을 제공합니다.

**엔진:** Unreal Engine 5.6 | **작성자:** prayslaks | **상태:** Beta

**English documentation: [README.md](./README.md)**

## ☕ 후원
이 프로젝트가 도움이 되셨다면, 커피 한 잔으로 개발을 응원해 주세요!

[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-Support%20Me-orange?style=flat-square&logo=buy-me-a-coffee)](https://www.buymeacoffee.com/prayslaks)

## 목차

- [빠른 시작 및 테스트](#-빠른-시작-및-테스트)
- [기능](#기능)
- [모듈 구조](#모듈-구조)
- [아키텍처](#아키텍처)
- [핵심 클래스 목록](#핵심-클래스-목록)
- [사용법](#사용법)
  - [C++](#c)
- [Job Handle](#job-handle)
- [HTTP 응답 모드](#http-응답-모드)
  - [Raw 응답](#raw-응답-sendrequest_rawresponse)
  - [Custom 응답](#custom-응답-sendrequest_customresponse)
  - [401 자동 갱신 흐름](#401-자동-갱신-흐름)
  - [토큰 보안](#토큰-보안)
- [확장 가능한 호스트 설정](#확장-가능한-호스트-설정)
- [콘솔 변수](#콘솔-변수)
- [테스트 서버 (FastAPI)](#테스트-서버-fastapi)
- [파일 구조](#파일-구조)
- [라이선스](#라이선스)

## 🚀 빠른 시작 및 테스트

이 플러그인에는 즉시 테스트에 필요한 모든 것이 포함되어 있습니다:
* **테스트 서버:** Python 3.14.3 기반 서버(`main.py`)가 포함되어 있습니다.
* **빌드된 실행 파일:** 바로 실행 가능한 `.exe`가 제공됩니다.
* **인엔진 에셋:** 플러그인 콘텐츠 폴더에 전용 **테스트 레벨**과 **UMG**가 포함되어 있습니다.

## 기능

- JWT Access/Refresh Token 관리 (Windows DPAPI 암호화)
- HTTP Request Job을 통한 자동 HTTP 요청 재시도 (5xx, 타임아웃, 네트워크 오류)
- 401 응답 시 자동 토큰 갱신 및 요청 재시도 큐
- ServiceType별 호스트 URL/토큰 분리 및 호스트 설정 확장성 (`GameServer`, `AuthServer`, `etc`)
- Raw HTTP 응답 지원
- Custom HTTP 응답 정규화 지원 (non-2xx → 일관된 JSON 구조)
- C++ 템플릿 API (`CallApi_Template<T>`) 및 Blueprint 지원
- Blueprint 와일드카드 구조체 파싱 (`CustomThunk`): JSON ↔ USTRUCT 변환
- 테스트 서버 API에 대응하는 사전 정의 요청/응답 구조체 (`FJWNU_REQ_*`, `FJWNU_RES_*`)
- Request Job Handle (`UJWNU_HttpRequestJobHandle`): `Cancel`, `IsRunning`, `IsCancelled`를 C++ 및 Blueprint에 노출; 401 토큰 리프레시 사이클 동안에도 핸들 유효성 유지

## 모듈 구조

| 모듈 | 타입 | 설명 |
|---|---|---|
| `JWNetworkUtility` | Runtime | 핵심 플러그인 — HTTP Job, HTTP Client, API Client, Token Provider, Host Provider |
| `JWNetworkUtilityTest` | Runtime | 테스트/데모 모듈 — API 테스트 액터 |

## 아키텍처

![img.png](./Resources/Architecture.png)

## 핵심 클래스 목록

| 클래스 | 베이스 | 역할 |
|---|---|---|
| `UJWNU_GIS_ApiClientService` | GameInstanceSubsystem | 상위 API: 템플릿 파싱, 401 갱신 큐 |
| `UJWNU_GIS_HttpClientHelper` | GameInstanceSubsystem | 하위 HTTP: raw/정규화 응답 |
| `UJWNU_GIS_HttpRequestJobProcessor` | GameInstanceSubsystem | Job 생성, 쿼리 파라미터 인코딩 |
| `UJWNU_GIS_ApiIdentityProvider` | GameInstanceSubsystem | 토큰 + UserId/SessionId 저장, DPAPI 암호화 |
| `UJWNU_GIS_ApiHostProvider` | GameInstanceSubsystem | INI 설정 기반 호스트 URL |
| `UJWNU_HttpRequestJob` | UObject | 단일 요청 수명주기: 재시도, 타임아웃, 취소 |
| `UJWNU_HttpRequestJobHandle` | UObject (BlueprintType) | 논리적 요청 핸들: 401 리프레시 생존, `Cancel`/`IsRunning`/`IsCancelled` 노출 |
| `UJWNU_BFL_ApiClientService` | BlueprintFunctionLibrary | Blueprint 노출 API |
| `UJWNU_BFL_AuthWidgetHelper` | BlueprintFunctionLibrary | 인증 위젯 유효성 검사 헬퍼 (이메일, 비밀번호) |

## 사용법

### C++

```cpp
const auto Callback =
    FOnHttpRequestCompletedDelegate::CreateLambda([](const int32 StatusCode, const FString& ResponseBody)
    {
        PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("Raw Response From Server : %s"), *ResponseBody);
    });

FString ProvidedHost;
if (UJWNU_GIS_ApiHostProvider::Get(GetWorld())->GetHost(EJWNU_ServiceType::AuthServer, ProvidedHost))
{
    return;
}

const FString URL = ProvidedHost + TEXT("/health");
const FString AuthToken = {};
const FString ContentBody = {};
const TMap<FString, FString> QueryParams = {};

UJWNU_GIS_HttpClientHelper::SendRequest_RawResponse(
    GetWorld(),
    EJWNU_HttpMethod::Get,
    URL,
    AuthToken,
    ContentBody,
    QueryParams,
    Callback);
```

```cpp
const auto Callback =
    [](const FJWNU_RES_Base& Response)
    {
        PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("Custom Reseponse From Server : %s %s"), *Response.Code, *Response.Message);
    };

const FString Endpoint = TEXT("/health");
const FString ContentBody = {};
const TMap<FString, FString> QueryParams = {};

UJWNU_HttpRequestJobHandle* Handle = UJWNU_GIS_ApiClientService::CallApi_Template<FJWNU_RES_Base>(
    GetWorld(),
    EJWNU_HttpMethod::Get,
    EJWNU_ServiceType::AuthServer,
    Endpoint,
    ContentBody,
    QueryParams,
    Callback);
// Handle->Cancel(), Handle->IsRunning(), Handle->IsCancelled()
```

```cpp
const auto Callback =
    FOnHttpResponseDelegate::CreateLambda([](const EJWNU_HttpStatusCode StatusCode, const FString& ResponseBody)
    {
        PRINT_LOG(LogJWNU_Actor_ApiTest, Display, TEXT("Raw Response From Server : %s"), *ResponseBody);
    });

const FString Endpoint = TEXT("/health");
const FString ContentBody = {};
const TMap<FString, FString> QueryParams = {};

UJWNU_HttpRequestJobHandle* Handle = UJWNU_GIS_ApiClientService::CallApi_NoTemplate(
    GetWorld(),
    EJWNU_HttpMethod::Get,
    EJWNU_ServiceType::AuthServer,
    Endpoint,
    ContentBody,
    QueryParams,
    Callback);
// Handle->Cancel(), Handle->IsRunning(), Handle->IsCancelled()
```

### Blueprint

언리얼 에디터로 (`Plugins/JWNetworkUtility/Content`) 내 테스트 레벨과 위젯 블루프린트를 확인해주세요.
![img.png](./Resources/BlueprintExample_0.png)
![img.png](./Resources/BlueprintExample_1.png)
![img.png](./Resources/BlueprintExample_2.png)

## Job Handle

`UJWNU_HttpRequestJobHandle`는 HTTP 요청의 논리적 수명을 추적하는 `BlueprintType` UObject입니다. `CallApi` / `SendHttpRequest` 호출당 하나의 핸들이 생성되며, 401 토큰 리프레시 사이클 동안에도 유효한 상태를 유지합니다. C++과 Blueprint 모두에서 요청을 제어하거나 상태를 조회하는 데 사용합니다.

| 메서드 | Blueprint | 설명 |
|---|---|---|
| `Cancel()` | `BlueprintCallable` | 진행 중인 요청을 취소 |
| `IsRunning()` | `BlueprintPure` | Job이 실행 중이거나 401 리프레시 대기 중이면 `true` |
| `IsCancelled()` | `BlueprintPure` | 요청이 취소됐으면 `true` |

**반환 체인:**

| 호출자 | 반환값 |
|---|---|
| `BFL::SendHttpRequest` / `BFL::CallApi` | `Handle*` |
| `ApiClientService::CallApi_Template` / `CallApi_NoTemplate` | `Handle*` |
| `HttpClientHelper::SendRequest_*` | `Job*` (저수준 전송 계층, Handle 없음) |

**401 리프레시 수명주기:**

1. 401 감지 → `Handle->MarkWaitingForRefresh()` → `IsRunning()`이 `true` 유지
2. 리프레시 성공 → 새 Job 생성 → `Handle->BindJob(newJob)` → 새 토큰으로 원래 요청 재전송
3. 리프레시 중 `Handle->Cancel()` → 드레인 콜백에서 재시도 건너뜀; `IsCancelled()`가 `true` 반환

**Blueprint:** `SendHttpRequest` 및 `CallApi` 노드는 `Handle` 반환 핀을 노출합니다. 이를 `Cancel`, `IsRunning`, `IsCancelled` 노드에 연결하여 Blueprint에서 요청 수명주기를 제어할 수 있습니다.

## HTTP 응답 모드

`UJWNU_GIS_HttpClientHelper`는 두 가지 응답 모드를 제공합니다. 서버 아키텍처에 맞는 모드를 선택하세요.

### Raw 응답 (`SendRequest_RawResponse`)

완전히 범용적인 모드입니다. 서버의 HTTP 응답 본문이 **변환 없이** 콜백에 전달됩니다. 다음과 같은 경우 사용합니다:
- 서버가 이미 잘 정의된 JSON 구조를 반환하여 직접 처리하고 싶을 때
- 응답 파싱에 대한 완전한 제어가 필요할 때 (바이너리 데이터, 비-JSON 포맷, 서드파티 API 등)

```
서버 응답 (모든 포맷) ──► 콜백이 본문을 그대로 수신
```

### Custom 응답 (`SendRequest_CustomResponse`)

**모든** 응답을 일관된 `{success, code, message}` JSON 구조로 변환하는 정규화 모드입니다. 서버가 다음과 같은 규약을 따를 때 유용합니다:

```json
{"Success": true, "Code": "CODE", "Message": "message"}
```

**동작 방식:**

| 조건 | 콜백이 수신하는 값 |
|---|---|
| **2xx** | 서버의 원본 JSON (그대로 전달) |
| **non-2xx** | `{"success": false, "code": "<CUSTOM_CODE>", "message": "<CUSTOM_MESSAGE>"}` |
| **네트워크 오류** | `{"success": false, "code": "NETWORK_ERROR", "message": "Failed to Send HTTP Request"}` |

**사용 이유:**

- **관심사 분리:** 네트워크 수준 실패(타임아웃, DNS, 연결)와 서비스 수준 실패(400, 403, 500)가 명확히 구분됩니다. 게임 로직에서는 raw HTTP 상태 코드를 검사하지 않고 `success`와 `code`만 확인하면 됩니다.
- **커스텀 코드 매핑:** `UJWNU_GIS_HttpClientHelper` 내부의 `StatusCodeToCustomCodeMap` / `StatusCodeToCustomMessageMap`이 HTTP 상태 코드를 애플리케이션별 코드로 변환합니다 (예: `401 → "UNAUTHORIZED"`, `503 → "SERVICE_UNAVAILABLE"`). 이 맵을 수정하여 고유한 코드를 정의하고 다양한 에러 코드를 다른 UI 흐름이나 재시도 전략으로 라우팅할 수 있습니다.
- **균일한 파싱:** 성공이든 실패든 모든 응답이 동일한 JSON 형태를 공유하므로, 단일 USTRUCT(`FJWNU_RES_Base` 등)로 모든 결과를 역직렬화할 수 있습니다.

**트레이드오프:**

- non-2xx 응답의 원본 HTTP 응답 본문은 **폐기**됩니다. 서버가 non-2xx 본문에 의미 있는 에러 상세 정보를 포함한다면 Raw 응답 모드가 더 적합합니다.
- 정규화 로직은 특정 방식을 따릅니다. 서버가 다른 envelope 구조를 사용한다면 `JWNU_GIS_HttpClientHelper.cpp`의 `SendRequest_CustomResponse`를 서버 규약에 맞게 수정해야 합니다.

### 401 자동 갱신 흐름

1. API 호출이 401을 반환
2. 요청이 `PendingJobQueues[ServiceType]`에 큐잉
3. ServiceType별 단일 갱신 요청 (`RefreshInProgressFlags`)
4. `/auth/refresh` 엔드포인트 호출
5. 성공 시: 토큰 갱신, 큐 배출 (대기 중인 모든 요청 재전송)
6. 실패 시: 에러와 함께 큐 배출

### 토큰 보안

- **Access Token**: `FJWNU_AccessTokenContainer`에 메모리 내 저장 (ServiceType별, `ExpiresAt` 포함)
- **Refresh Token**: `FJWNU_RefreshTokenContainer`에 저장, Windows DPAPI (`CryptProtectData`/`CryptUnprotectData`)로 암호화, Device ID 엔트로피 솔트 사용, `Saved/Config/JWNetworkUtility/auth_{ServiceType}.bin`에 저장
- **UserId**: 메모리 전용 FString (로그인/갱신 응답에서 설정, `GetUserId`/`SetUserId`로 관리)

## 확장 가능한 호스트 설정

### 호스트 URL (`Config/DefaultJWNetworkUtility.ini`)

```ini
[/Script/JWNetworkUtility.JWNU_GIS_ApiHostProvider]
GameServer="127.0.0.1:5000"
AuthServer="127.0.0.1:5000"
```

호스트 프로젝트에서 오버라이드: 프로젝트의 `DefaultJWNetworkUtility.ini`에 동일한 섹션을 추가합니다.

### UENUM EJWNU_ServiceType (`Source/JWNetworkUtility/Public/JWNetworkUtilityTypes.h`)

```cpp
UENUM(BlueprintType)
enum class EJWNU_ServiceType : uint8
{
    GameServer,
    AuthServer,
};
```

### UJWNU_GIS_ApiHostProvider::Initialize (`Source/JWNetworkUtility/Private/JWNU_GIS_ApiHostProvider`)

```cpp
void UJWNU_GIS_ApiHostProvider::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const FString Section = TEXT("/Script/JWNetworkUtility.JWNU_GIS_ApiHostProvider");
	FString PluginDir = IPluginManager::Get().FindPlugin(TEXT("JWNetworkUtility"))->GetBaseDir();
	const FString ConfigPath = FPaths::Combine(PluginDir, TEXT("Config"), TEXT("DefaultJWNetworkUtility.ini"));
	PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Display, TEXT("호스트 주소 로드 시도 : %s"), *ConfigPath);

	auto TryLoad = [&](const EJWNU_ServiceType Type, const TCHAR* Key)
	{
		if (FString Value; GConfig->GetString(*Section, Key, Value, ConfigPath))
		{
			ServiceTypeToHostMap.Add(Type, Value);
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Display, TEXT("호스트 주소 로드 완료 — %s : %s"), Key, *Value);
		}
		else
		{
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("호스트 주소 로드 실패 - %s : ???"), Key);
		}
	};

	TryLoad(EJWNU_ServiceType::GameServer, TEXT("GameServer"));
	TryLoad(EJWNU_ServiceType::AuthServer, TEXT("AuthServer"));
}
```

## 콘솔 변수

| CVar | 기본값 | 설명 |
|---|---|---|
| `JWNU.DebugScreen` | `false` | 토큰 갱신 흐름에 대한 화면 디버그 메시지 토글 |

## 테스트 서버 (FastAPI)

### 빠른 시작 (빌드된 실행 파일)

Python 환경 설정 없이 바로 실행할 수 있는 빌드 파일이 포함되어 있습니다.

```bash
TestServer/dist/main.exe
```

### 소스에서 실행

```bash
cd TestServer
cp .env.example .env   # 필요에 따라 수정
pip install -r requirements.txt
uvicorn main:app --reload --port 5000
```

### 환경 변수 (`.env`)

| 변수 | 기본값 | 설명 |
|---|---|---|
| `LOG_LANG` | `ko` | 서버 로그 언어 (`ko` / `en`) |
| `SMTP_HOST` | *(비어있음)* | SMTP 호스트 (미설정 시 인증 코드가 콘솔에 출력) |
| `SMTP_PORT` | `587` | SMTP 포트 |
| `SMTP_USER` | *(비어있음)* | SMTP 계정 |
| `SMTP_PASSWORD` | *(비어있음)* | SMTP 비밀번호 |
| `SMTP_FROM` | `SMTP_USER` | 발신자 이메일 주소 |

### 엔드포인트

| 메서드 | 경로 | 인증 | 설명 |
|---|---|---|---|
| GET | `/health` | 불필요 | 헬스 체크 |
| GET | `/timeout` | 불필요 | 타임아웃 시뮬레이션 (`?second=N`, 최대 60초) |
| POST | `/auth/register/send-code` | 불필요 | 이메일 인증 코드 발송 |
| POST | `/auth/register/verify-code` | 불필요 | 코드 인증 |
| POST | `/auth/register` | 불필요 | 회원가입 완료 |
| POST | `/auth/login` | 불필요 | 로그인 (JWT 발급) |
| POST | `/auth/refresh` | 불필요 | 토큰 갱신 (일회용 리프레시 토큰) |
| POST | `/auth/logout` | 필요 | 로그아웃 (액세스 토큰 블랙리스트, 리프레시 토큰 폐기) |
| POST | `/auth/reset` | 불필요 | 모든 사용자, 인증, 토큰 초기화 |
| GET/POST/PUT/DELETE | `/api/data` | 필요 | CRUD 테스트 (`?delay=N&status=CODE` 시뮬레이션 지원) |
| GET | `/debug/users/registered` | 불필요 | 등록된 사용자 목록 (개발용) |
| GET | `/debug/users/active` | 불필요 | 활성 세션 (개발용) |
| GET | `/debug/verifications` | 불필요 | 인증 코드 상태 (개발용) |

## 파일 구조

```
JWNetworkUtility/
├── Config/DefaultJWNetworkUtility.ini
├── Content/
├── Resources/Icon128.png
├── Source/
│   ├── JWNetworkUtility/              (Runtime)
│   │   ├── JWNetworkUtility.Build.cs
│   │   ├── Public/
│   │   │   ├── JWNetworkUtility.h
│   │   │   ├── JWNetworkUtilityTypes.h
│   │   │   ├── JWNetworkUtilityDelegates.h
│   │   │   ├── JWNU_GIS_ApiClientService.h
│   │   │   ├── JWNU_GIS_HttpClientHelper.h
│   │   │   ├── JWNU_GIS_HttpRequestJobProcessor.h
│   │   │   ├── JWNU_GIS_ApiIdentityProvider.h
│   │   │   ├── JWNU_GIS_ApiHostProvider.h
│   │   │   ├── JWNU_GIS_CustomCodeHelper.h
│   │   │   ├── JWNU_GIS_SteamWorks.h
│   │   │   ├── JWNU_HttpRequestJob.h
│   │   │   ├── JWNU_HttpRequestJobHandle.h
│   │   │   ├── JWNU_BFL_ApiClientService.h
│   │   │   └── JWNU_BFL_AuthWidgetHelper.h
│   │   └── Private/
│   │       ├── JWNetworkUtility.cpp
│   │       ├── JWNetworkUtilityTypes.cpp
│   │       ├── JWNetworkUtilityDelegates.cpp
│   │       ├── JWNU_GIS_ApiClientService.cpp
│   │       ├── JWNU_GIS_HttpClientHelper.cpp
│   │       ├── JWNU_GIS_HttpRequestJobProcessor.cpp
│   │       ├── JWNU_GIS_ApiIdentityProvider.cpp
│   │       ├── JWNU_GIS_ApiHostProvider.cpp
│   │       ├── JWNU_GIS_CustomCodeHelper.cpp
│   │       ├── JWNU_GIS_SteamWorks.cpp
│   │       ├── JWNU_HttpRequestJob.cpp
│   │       ├── JWNU_HttpRequestJobHandle.cpp
│   │       ├── JWNU_BFL_ApiClientService.cpp
│   │       └── JWNU_BFL_AuthWidgetHelper.cpp
│   └── JWNetworkUtilityTest/          (Runtime, JWNetworkUtility 의존)
│       ├── JWNetworkUtilityTest.Build.cs
│       ├── Public/
│       │   ├── JWNetworkUtilityTest.h
│       │   └── JWNU_Actor_ApiTest.h
│       └── Private/
│           ├── JWNetworkUtilityTest.cpp
│           └── JWNU_Actor_ApiTest.cpp
├── TestServer/
│   ├── main.py
│   ├── requirements.txt
│   ├── .env.example
│   ├── openapi.json
│   └── dist/
│       └── main.exe
├── Doxygen/
├── Analysis/                      (gitignored, 분석 산출물)
├── JWNetworkUtility.uplugin
├── CLAUDE.md
├── LICENSE
├── README_ko.md
└── README.md
```

## 라이선스

`LICENSE` 파일을 참고하세요.
