# JWNetworkUtility Plugin

A standalone Unreal Engine 5.6 plugin providing a layered HTTP API client system with JWT authentication, automatic token refresh, retry/timeout handling, and Blueprint support.

**Engine:** Unreal Engine 5.6 | **Author:** prayslaks | **Status:** Beta

## Features

- JWT Access/Refresh Token management (Windows DPAPI encryption)
- Automatic token refresh and request retry queue on 401 responses
- Per-ServiceType host URL/token separation (`GameServer`, `AuthServer`, `Platform`, `External`)
- HTTP response normalization (non-2xx → consistent JSON structure)
- Request retry (5xx, timeout, network errors)
- C++ template API (`CallApi_Template<T>`) and Blueprint support
- Blueprint wildcard struct parsing (`CustomThunk`)

## Module Structure

| Module | Type | Description |
|---|---|---|
| `JWNetworkUtility` | Runtime | Core plugin — HTTP client, token management, Blueprint API |
| `JWNetworkUtilityTest` | Runtime | Test/demo module — API test actor, auth widget helpers |

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  Blueprint Layer                                     │
│  UJWNU_BFL_ApiClientService (BlueprintFunctionLib)   │
│    - CallApi, SendHttpRequest, ConvertJsonStringTo   │
│      Struct (CustomThunk wildcard)                   │
├─────────────────────────────────────────────────────┤
│  Service Layer                                       │
│  UJWNU_GIS_ApiClientService (GameInstanceSubsystem)  │
│    - CallApi_Template<T>() / CallApi_NoTemplate()    │
│    - 401 auto-refresh with per-ServiceType queue     │
│    - Host & token resolution from providers          │
├─────────────────────────────────────────────────────┤
│  Provider Layer                                      │
│  UJWNU_GIS_ApiIdentityProvider                       │
│    — AccessTokenContainer + RefreshTokenContainer    │
│    — UserId/SessionId, DPAPI encryption              │
│  UJWNU_GIS_ApiHostProvider   — Host URL config (INI) │
├─────────────────────────────────────────────────────┤
│  Transport Layer                                     │
│  UJWNU_GIS_HttpClientHelper (GameInstanceSubsystem)  │
│    - Response normalization (non-2xx → custom JSON)  │
│  UJWNU_GIS_HttpRequestJobProcessor                   │
│    - Creates UJWNU_HttpRequestJob                    │
│  UJWNU_HttpRequestJob (UObject)                      │
│    - Retry, timeout, cancellation                    │
├─────────────────────────────────────────────────────┤
│  Utility                                             │
│  UJWNU_GIS_CustomCodeHelper  — Error code → FText    │
│  UJWNU_GIS_SteamWorks        — Steam auth tickets    │
└─────────────────────────────────────────────────────┘
```

### Class List

| Class | Base | Role |
|---|---|---|
| `UJWNU_GIS_ApiClientService` | GameInstanceSubsystem | High-level API: template parsing, 401 refresh queue |
| `UJWNU_GIS_HttpClientHelper` | GameInstanceSubsystem | Low-level HTTP: raw/normalized responses |
| `UJWNU_GIS_HttpRequestJobProcessor` | GameInstanceSubsystem | Job creation, query param encoding |
| `UJWNU_GIS_ApiIdentityProvider` | GameInstanceSubsystem | Token + UserId/SessionId storage, DPAPI encryption |
| `UJWNU_GIS_ApiHostProvider` | GameInstanceSubsystem | Host URLs from INI config |
| `UJWNU_GIS_CustomCodeHelper` | GameInstanceSubsystem | HTTP status → localized FText |
| `UJWNU_GIS_SteamWorks` | GameInstanceSubsystem | Steam auth ticket |
| `UJWNU_HttpRequestJob` | UObject | Single request lifecycle: retry, timeout, cancel |
| `UJWNU_BFL_ApiClientService` | BlueprintFunctionLibrary | Blueprint-exposed API |

### Test Module Classes (JWNetworkUtilityTest)

| Class | Base | Role |
|---|---|---|
| `AJWNU_Actor_ApiTest` | AActor | API test/demo actor |
| `UJWNU_BFL_AuthWidget` | BlueprintFunctionLibrary | Auth widget validation helpers (email, password) |

## Usage

### C++ Template API Call

```cpp
UJWNU_GIS_ApiClientService::CallApi_Template<FMyStruct>(
    WorldContext, EJWNU_HttpMethod::Get, EJWNU_ServiceType::GameServer,
    TEXT("/api/data"), TEXT(""), TMap<FString,FString>(),
    [](const FMyStruct& Response) { /* typed result */ }
);
```

### Response Normalization

All HTTP responses are normalized to a consistent JSON structure.

- **2xx responses**: Original JSON from server passed through
- **non-2xx responses**: Converted to:
  ```json
  {"success": false, "code": "<HTTP_STATUS>", "message": "<STATUS_TEXT>"}
  ```

Server-side response format:
```json
{"Success": true, "Code": "CODE", "Message": "message"}
```

### 401 Auto-Refresh Flow

1. API call returns 401
2. Request queued in `PendingJobQueues[ServiceType]`
3. Single refresh request per ServiceType (`RefreshInProgressFlags`)
4. Calls `/auth/refresh` endpoint
5. On success: update token, drain queue (re-send all pending)
6. On failure: drain queue with error

### Token Security

- **Access Token**: Stored in-memory in `FJWNU_AccessTokenContainer` (per-ServiceType, with `ExpiresAt`)
- **Refresh Token**: Stored in `FJWNU_RefreshTokenContainer`, encrypted via Windows DPAPI (`CryptProtectData`/`CryptUnprotectData`), saved to `Saved/Config/auth_{ServiceType}.bin`
- **UserId**: Memory-only FString (set from login/refresh response)

## Configuration

### Host URL (`Config/DefaultJWNetworkUtility.ini`)

```ini
[/Script/JWNetworkUtility.JWNU_GIS_ApiHostProvider]
GameServer="127.0.0.1:5000"
AuthServer="127.0.0.1:5000"
```

Override in host project: add the same section to the project's `DefaultJWNetworkUtility.ini`.

### Console Variables

| CVar | Default | Description |
|---|---|---|
| `JWNU.DebugScreen` | `false` | Toggle on-screen debug messages for the token refresh flow |

## Test Server (FastAPI)

```bash
cd TestServer
cp .env.example .env   # edit as needed
pip install -r requirements.txt
uvicorn main:app --reload --port 5000
```

### Environment Variables (`.env`)

| Variable | Default | Description |
|---|---|---|
| `LOG_LANG` | `ko` | Server log language (`ko` / `en`) |
| `SMTP_HOST` | *(empty)* | SMTP host (if unset, auth codes printed to console) |
| `SMTP_PORT` | `587` | SMTP port |
| `SMTP_USER` | *(empty)* | SMTP account |
| `SMTP_PASSWORD` | *(empty)* | SMTP password |
| `SMTP_FROM` | `SMTP_USER` | Sender email address |

### Endpoints

| Method | Path | Auth | Description |
|---|---|---|---|
| GET | `/health` | No | Health check |
| POST | `/auth/register/send-code` | No | Send email verification code |
| POST | `/auth/register/verify-code` | No | Verify code |
| POST | `/auth/register` | No | Complete registration |
| POST | `/auth/login` | No | Login (JWT issued) |
| POST | `/auth/refresh` | No | Token refresh |
| POST | `/auth/reset` | No | Clear all users, verifications, tokens |
| GET/POST/PUT/DELETE | `/api/data` | Yes | CRUD test |
| GET | `/debug/users` | No | List registered users (dev) |
| GET | `/debug/verifications` | No | Verification code status (dev) |

## File Structure

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
│   │   │   └── JWNU_BFL_ApiClientService.h
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
│   │       └── JWNU_BFL_ApiClientService.cpp
│   └── JWNetworkUtilityTest/          (Runtime, depends on JWNetworkUtility)
│       ├── JWNetworkUtilityTest.Build.cs
│       ├── Public/
│       │   ├── JWNetworkUtilityTest.h
│       │   ├── JWNU_Actor_ApiTest.h
│       │   └── JWNU_BFL_AuthWidget.h
│       └── Private/
│           ├── JWNetworkUtilityTest.cpp
│           ├── JWNU_Actor_ApiTest.cpp
│           └── JWNU_BFL_AuthWidget.cpp
├── TestServer/
│   ├── main.py
│   ├── requirements.txt
│   ├── .env.example
│   └── openapi.json
├── JWNetworkUtility.uplugin
├── CLAUDE.md
├── LICENSE
└── README.md
```

## License

See `LICENSE` file.

---

# JWNetworkUtility 플러그인 (한국어)

Unreal Engine 5.6용 HTTP API 클라이언트 플러그인입니다.
JWT 인증, 자동 토큰 리프레시, 재시도/타임아웃, Blueprint 지원을 제공합니다.

## 주요 기능

- JWT Access/Refresh Token 관리 (Windows DPAPI 암호화)
- 401 응답 시 자동 토큰 리프레시 및 요청 재시도 큐
- ServiceType별 호스트 URL/토큰 분리 (`GameServer`, `AuthServer`, `Platform`, `External`)
- HTTP 응답 정규화 (non-2xx → 일관된 JSON 구조)
- 요청 재시도 (5xx, 타임아웃, 네트워크 에러)
- C++ 템플릿 API (`CallApi_Template<T>`) 및 Blueprint 지원
- Blueprint Wildcard Struct 파싱 (`CustomThunk`)

## 모듈 구조

| 모듈 | 타입 | 설명 |
|---|---|---|
| `JWNetworkUtility` | Runtime | 핵심 플러그인 — HTTP 클라이언트, 토큰 관리, Blueprint API |
| `JWNetworkUtilityTest` | Runtime | 테스트/데모 모듈 — API 테스트 액터, 인증 위젯 헬퍼 |

## 사용법

### C++ 템플릿 API 호출

```cpp
UJWNU_GIS_ApiClientService::CallApi_Template<FMyStruct>(
    WorldContext, EJWNU_HttpMethod::Get, EJWNU_ServiceType::GameServer,
    TEXT("/api/data"), TEXT(""), TMap<FString,FString>(),
    [](const FMyStruct& Response) { /* 타입 지정된 결과 */ }
);
```

### 응답 정규화

모든 HTTP 응답은 일관된 JSON 구조로 정규화됩니다.

- **2xx 응답**: 서버가 반환한 원본 JSON을 그대로 전달
- **non-2xx 응답**: 아래 형식의 JSON으로 변환하여 전달
  ```json
  {"success": false, "code": "<HTTP_STATUS>", "message": "<STATUS_TEXT>"}
  ```

서버 측 응답 형식:
```json
{"Success": true, "Code": "CODE", "Message": "message"}
```

### 401 자동 리프레시 흐름

1. API 호출 → 401 응답 수신
2. 요청을 `PendingJobQueues[ServiceType]`에 큐잉
3. ServiceType당 단일 리프레시 요청 발생 (`RefreshInProgressFlags`)
4. `/auth/refresh` 엔드포인트 호출
5. 성공 시: 토큰 갱신 → 큐의 모든 대기 요청 재전송
6. 실패 시: 큐의 모든 요청에 에러 전달

### 토큰 보안

- **Access Token**: `FJWNU_AccessTokenContainer`에 메모리 저장 (ServiceType별, `ExpiresAt` 포함)
- **Refresh Token**: `FJWNU_RefreshTokenContainer`에 저장, Windows DPAPI(`CryptProtectData`/`CryptUnprotectData`)로 암호화 후 `Saved/Config/auth_{ServiceType}.bin`에 파일 저장
- **UserId**: 메모리 전용 FString (로그인/리프레시 응답에서 설정)

## 설정

### Host URL (`Config/DefaultJWNetworkUtility.ini`)

```ini
[/Script/JWNetworkUtility.JWNU_GIS_ApiHostProvider]
GameServer="127.0.0.1:5000"
AuthServer="127.0.0.1:5000"
```

호스트 프로젝트에서 오버라이드: 프로젝트의 `DefaultJWNetworkUtility.ini`에 동일 섹션 추가.

### 콘솔 변수

| CVar | 기본값 | 설명 |
|---|---|---|
| `JWNU.DebugScreen` | `false` | 토큰 리프레시 흐름의 온스크린 디버그 메시지 토글 |

## 테스트 서버 (FastAPI)

```bash
cd TestServer
cp .env.example .env   # 필요 시 편집
pip install -r requirements.txt
uvicorn main:app --reload --port 5000
```

### 환경 변수 (`.env`)

| 변수 | 기본값 | 설명 |
|---|---|---|
| `LOG_LANG` | `ko` | 서버 로그 언어 (`ko` / `en`) |
| `SMTP_HOST` | *(빈값)* | SMTP 호스트 (미설정 시 인증코드 콘솔 출력) |
| `SMTP_PORT` | `587` | SMTP 포트 |
| `SMTP_USER` | *(빈값)* | SMTP 계정 |
| `SMTP_PASSWORD` | *(빈값)* | SMTP 비밀번호 |
| `SMTP_FROM` | `SMTP_USER` | 발신 이메일 주소 |

### 엔드포인트

| 메서드 | 경로 | 인증 | 설명 |
|---|---|---|---|
| GET | `/health` | X | 헬스체크 |
| POST | `/auth/register/send-code` | X | 이메일 인증코드 발송 |
| POST | `/auth/register/verify-code` | X | 인증코드 검증 |
| POST | `/auth/register` | X | 회원가입 완료 |
| POST | `/auth/login` | X | 로그인 (JWT 발급) |
| POST | `/auth/refresh` | X | 토큰 리프레시 |
| POST | `/auth/reset` | X | 전체 데이터 초기화 (유저, 인증코드, 토큰) |
| GET/POST/PUT/DELETE | `/api/data` | O | CRUD 테스트 |
| GET | `/debug/users` | X | 등록 유저 조회 (개발용) |
| GET | `/debug/verifications` | X | 인증코드 상태 조회 (개발용) |
