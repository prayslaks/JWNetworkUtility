# CLAUDE.md — JWNetworkUtility Plugin

This file provides guidance to Claude Code when working with this plugin.

## Overview

JWNetworkUtility is a standalone Unreal Engine 5.6 plugin providing a layered HTTP API client system with JWT authentication, automatic token refresh, retry/timeout handling, and Blueprint support.

**Engine Version:** Unreal Engine 5.6
**Author:** prayslaks
**Status:** Beta

## Build

This is an Unreal plugin. Build via the host project:
```
Right-click <HostProject>.uproject → Generate Visual Studio project files → Build
```

Or command line:
```
"<UE5>/Engine/Build/BatchFiles/Build.bat" <Project> Win64 Development "<Project>.uproject" -waitmutex
```

### Test Server (FastAPI)

```bash
cd TestServer
pip install -r requirements.txt
uvicorn main:app --reload --port 5000
```

Endpoints: `/health`, `/auth/register/*`, `/auth/login`, `/auth/refresh`, `/auth/logout`, `/auth/reset`, `/api/data` (CRUD, JWT-protected, `?delay`/`?status` simulation)

## Module Structure

```
Source/
├── JWNetworkUtility/              (Runtime, LoadingPhase: Default)
│   ├── JWNetworkUtility.Build.cs
│   ├── Public/                    # 13 headers
│   └── Private/                   # 13 implementations
└── JWNetworkUtilityTest/          (Runtime, LoadingPhase: Default, depends on JWNetworkUtility)
    ├── JWNetworkUtilityTest.Build.cs
    ├── Public/                    # 3 headers
    └── Private/                   # 3 implementations
```

### JWNetworkUtility (Core)

**Dependencies:** Core, Projects, HTTP, Json, JsonUtilities, OnlineSubsystem, OnlineSubsystemSteam, OnlineSubsystemUtils, CoreUObject, Engine, Slate, SlateCore

**Plugin Dependencies:** OnlineSubsystem, OnlineSubsystemSteam

### JWNetworkUtilityTest (Test/Demo)

**Dependencies:** Core, Projects, HTTP, Json, JsonUtilities, OnlineSubsystem, OnlineSubsystemSteam, OnlineSubsystemUtils, CoreUObject, Engine, UMG, Slate, SlateCore, JWNetworkUtility

Contains test actors and auth widget Blueprint helpers. Separated from the core module so test/demo code is not shipped with the plugin runtime.

### Editor Module (K2Node — planned, not yet in source)

An editor module `JWNetworkUtilityEditor` was designed for a custom K2Node (`UJWNU_K2Node_CallApi`) that provides async Blueprint nodes with automatic struct parsing. Intermediate build artifacts exist but source files are not yet present. See the Architecture section for the intended design.

## Architecture

### Layer Diagram

```
┌─────────────────────────────────────────────────────┐
│  Blueprint Layer                                     │
│  UJWNU_BFL_ApiClientService (BlueprintFunctionLib)   │
│    - CallApi → Handle*, SendHttpRequest → Handle*    │
│    - ConvertJsonStringToStruct / ConvertStructTo     │
│      JsonString (CustomThunk wildcard)               │
│    - GetUserId, SetUserId, ClearSession              │
├─────────────────────────────────────────────────────┤
│  Handle Layer                                        │
│  UJWNU_HttpRequestJobHandle (UObject, BlueprintType) │
│    - Cancel, IsRunning, IsCancelled                  │
│    - BindJob (internal: Job rebind on 401 refresh)   │
│    - WaitingForRefresh state management              │
├─────────────────────────────────────────────────────┤
│  Service Layer                                       │
│  UJWNU_GIS_ApiClientService (GameInstanceSubsystem)  │
│    - CallApi_Template<T>() → Handle*                 │
│    - CallApi_NoTemplate()  → Handle*                 │
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
│    - SendRequest_* → UJWNU_HttpRequestJob*           │
│  UJWNU_GIS_HttpRequestJobProcessor                   │
│    - ProcessHttpRequestJob → UJWNU_HttpRequestJob*   │
│  UJWNU_HttpRequestJob (UObject)                      │
│    - Retry, timeout, cancellation                    │
├─────────────────────────────────────────────────────┤
│  Utility                                             │
│  UJWNU_GIS_CustomCodeHelper  — Error code → FText    │
│  UJWNU_GIS_SteamWorks        — Steam auth tickets    │
└─────────────────────────────────────────────────────┘
```

### Class List (JWNetworkUtility)

| Class | Base | Role |
|---|---|---|
| `UJWNU_GIS_ApiClientService` | GameInstanceSubsystem | High-level API: template parsing, 401 refresh queue |
| `UJWNU_GIS_HttpClientHelper` | GameInstanceSubsystem | Low-level HTTP: raw/normalized responses |
| `UJWNU_GIS_HttpRequestJobProcessor` | GameInstanceSubsystem | Job creation, query param encoding |
| `UJWNU_GIS_ApiIdentityProvider` | GameInstanceSubsystem | Token + UserId/SessionId storage, DPAPI encryption |
| `UJWNU_GIS_ApiHostProvider` | GameInstanceSubsystem | Host URLs from INI config |
| `UJWNU_GIS_CustomCodeHelper` | GameInstanceSubsystem | HTTP status → localized FText (Korean default) |
| `UJWNU_GIS_SteamWorks` | GameInstanceSubsystem | Steam auth ticket |
| `UJWNU_HttpRequestJob` | UObject | Single request lifecycle: retry, timeout, cancel |
| `UJWNU_HttpRequestJobHandle` | UObject (BlueprintType) | Logical request handle: survives 401 refresh, exposes Cancel/IsRunning/IsCancelled |
| `UJWNU_BFL_ApiClientService` | BlueprintFunctionLibrary | Blueprint-exposed API |

### Class List (JWNetworkUtilityTest)

| Class | Base | Role |
|---|---|---|
| `AJWNU_Actor_ApiTest` | AActor | API test/demo actor |
| `UJWNU_BFL_AuthWidget` | BlueprintFunctionLibrary | Auth widget validation helpers (email, password) |

### Enum: EJWNU_ServiceType

`GameServer`, `AuthServer` — used to segregate tokens and host URLs per service.

### Enum: EJWNU_HttpMethod

`Get`, `Post`, `Put`, `Delete`

### Enum: EJWNU_HttpStatusCode

Blueprint-friendly HTTP status code mapping: `None`, `NetworkError`, `Timeout`, `ParseError`, 2XX (`OK`, `Created`, `Accepted`, `NoContent`), 3XX (`MovedPermanently`, `Found`, `NotModified`), 4XX (`BadRequest`, `Unauthorized`, `Forbidden`, `NotFound`, `TooManyRequests`, etc.), 5XX (`InternalServerError`, `BadGateway`, `ServiceUnavailable`, `GatewayTimeout`, etc.), `UnknownError`. Helper: `JWNU_IntToHttpStatusCode(int32)`.

### Enum: EJWNU_ConvertJsonToStructResult / EJWNU_ConvertStructToJsonResult

JSON ↔ USTRUCT conversion results: `Success`, `Fail`, `InvalidJSON`/`NoMatch`.

### Request/Response Structs (Test Server API)

| Struct | Fields | Description |
|---|---|---|
| `FJWNU_REQ_AuthRegisterSendCode` | Email | Send verification code |
| `FJWNU_REQ_AuthRegisterVerifyCode` | Email, Code | Verify code |
| `FJWNU_REQ_AuthRegister` | Email, Password, Code | Complete registration |
| `FJWNU_REQ_AuthLogin` | Email, Password | Login |
| `FJWNU_REQ_AuthLogout` | UserId, TargetServer, RefreshToken | Logout |
| `FJWNU_REQ_AuthRefresh` | UserId, TargetServer, RefreshToken | Token refresh |
| `FJWNU_REQ_Data` | Data | Generic data request |
| `FJWNU_RES_Base` | Success, Code, Message | Base response |
| `FJWNU_RES_AuthRefresh` | +(AccessToken, ExpiresAt, RefreshToken, RefreshTokenExpiresAt, UserId) | Auth response (inherits Base) |
| `FJWNU_RES_Data` | +(Data) | Data response (inherits Base) |

## Key Patterns

### Response Normalization

All HTTP responses are normalized to a consistent JSON structure. Non-2xx responses are converted to:
```json
{"success": false, "code": "<HTTP_STATUS>", "message": "<STATUS_TEXT>"}
```

The server is expected to return:
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

### Template API Call (C++)

```cpp
UJWNU_HttpRequestJobHandle* Handle = UJWNU_GIS_ApiClientService::CallApi_Template<FMyStruct>(
    WorldContext, EJWNU_HttpMethod::Get, EJWNU_ServiceType::GameServer,
    TEXT("/api/data"), TEXT(""), TMap<FString,FString>(),
    [](const FMyStruct& Response) { /* typed result */ }
);
// Handle->Cancel(), Handle->IsRunning(), Handle->IsCancelled()
```

### Token & Identity Security

- Access tokens stored in `FJWNU_AccessTokenContainer` (memory, per-ServiceType, with `ExpiresAt`)
- Refresh tokens stored in `FJWNU_RefreshTokenContainer` (with `ExpiresAt`), encrypted via Windows DPAPI (`CryptProtectData`/`CryptUnprotectData`) with Device ID entropy salt (`FPlatformMisc::GetDeviceId() + "JWNetworkUtility"`)
- Refresh token containers are JSON-serialized before DPAPI encryption, then saved to `Saved/Config/JWNetworkUtility/auth_{ServiceType}.bin`
- UserId: single FString (memory-only, set from login/refresh response, managed via `GetUserId`/`SetUserId`)
- `ClearSession(ServiceType)`: resets AccessToken for a ServiceType while preserving UserId

### Request Job Retry

`FJWNU_RequestConfig` controls retry behavior:
- Configurable retry on 5xx, timeout, network errors
- Timeout management via `FTimerHandle`

### Job Handle Pattern

`UJWNU_HttpRequestJobHandle` wraps the logical lifetime of a request. Handle is created once per call and survives 401 token refresh cycles.

**Return chain:**
```
BFL (SendHttpRequest)   → Handle* (BFL creates Handle, binds Job)
BFL (CallApi)           → Handle* (forwarded from ApiClientService)
ApiClientService        → Handle* (creates Handle, rebinds Job on 401)
HttpClientHelper        → Job*    (pass-through)
HttpRequestJobProcessor → Job*    (creates Job)
```

**401 refresh integration:**
1. 401 detected → `Handle->MarkWaitingForRefresh()` → `IsRunning()` stays true
2. Refresh success → `Handle->ClearWaitingForRefresh()` → new Job created → `Handle->BindJob(newJob)`
3. `Handle->Cancel()` during refresh → `IsCancelled()` true → retry skipped in `OnTokenReady`

**Blueprint usage:** `SendHttpRequest`/`CallApi` nodes return a `UJWNU_HttpRequestJobHandle*` output pin. Call `Cancel`, `IsRunning`, `IsCancelled` on the handle.

### Blueprint Wildcard Struct Parsing

`ConvertJsonStringToStruct()` and `ConvertStructToJsonString()` use `DECLARE_FUNCTION` (CustomThunk) for wildcard `CustomStructureParam` support, allowing any USTRUCT to be passed from Blueprint. `ConvertJsonStringToStruct` returns `EJWNU_ConvertJsonToStructResult`, `ConvertStructToJsonString` returns `EJWNU_ConvertStructToJsonResult`.

## Configuration

**`Config/DefaultJWNetworkUtility.ini`:**
```ini
[/Script/JWNetworkUtility.JWNU_GIS_ApiHostProvider]
GameServer="127.0.0.1:5000"
AuthServer="127.0.0.1:5000"
```

Host project overrides go in the project's `DefaultJWNetworkUtility.ini`.

### Console Variables

| CVar | Default | Description |
|---|---|---|
| `JWNU.DebugScreen` | `false` | Enable/disable on-screen debug messages for the token refresh flow. Toggle via console: `JWNU.DebugScreen 1` / `JWNU.DebugScreen 0` |

### On-Screen Debug Messages (JWNU.DebugScreen)

When enabled, `GEngine->AddOnScreenDebugMessage` is displayed at key token refresh flow points:

| Event | Color | Message |
|---|---|---|
| Access token expired → queue | Yellow | `Access Token Expired — Queuing refresh` |
| 401 response → refresh trigger | Orange | `401 Unauthorized — Triggering token refresh` |
| Refresh API called | Cyan | `Calling Refresh API → {URL}` |
| Refresh success → drain queue | Green | `Token Refresh SUCCESS — Draining N pending jobs` |
| Refresh failure → drain error | Red | `Token Refresh FAILED — {Code}: {Message}` |

Macro: `JWNU_SCREEN_DEBUG(Key, Duration, Color, fmt, ...)` (defined in `JWNetworkUtility.h`)

## K2Node Design (Editor Module — Planned)

Intended structure:
```
Source/JWNetworkUtilityEditor/        (UncookedOnly)
├── JWNetworkUtilityEditor.Build.cs   (depends: BlueprintGraph, KismetCompiler)
├── Public/
│   ├── JWNetworkUtilityEditorModule.h
│   └── JWNU_K2Node_CallApi.h
└── Private/
    ├── JWNetworkUtilityEditorModule.cpp
    └── JWNU_K2Node_CallApi.cpp
```

**UJWNU_K2Node_CallApi** (inherits UK2Node, NOT UK2Node_BaseAsyncTask):
- `ResponseStructType` UPROPERTY — user selects target struct in Details panel
- Dynamic output pin type changes when struct selection changes
- ExpandNode flow:
  ```
  [Exec] → [Factory: CallApi] → [AddDelegate: OnCompleted] → [AddDelegate: OnFailed] → [Activate] → [Then]
  [CustomEvent: OnCompleted(FString)] → [ConvertJsonStringToStruct] → [OnCompleted Exec + typed OutStruct pin]
  [CustomEvent: OnFailed(FString)] → [OnFailed Exec]
  ```
- Uses companion `UJWNU_AsyncAction_CallApi` (UCancellableAsyncActionBase, BlueprintInternalUseOnly)
- CustomEvent params added via `CreateUserDefinedPin`
- Wildcard pin type set manually in ExpandNode: `PinType.PinSubCategoryObject = StructType`

## Known Issues / TODO

- Steam auth ticket obtained but not transmitted to server (`SteamWorks.cpp`)
- Editor module source files need to be (re)created
- `FJWNU_REQ_AuthLogout` has typo in USTRUCT/UPROPERTY macros (`BLueprintType`, `editAnywhere`) — compiles but non-standard casing

## File Tree

```
JWNetworkUtility/
├── Config/DefaultJWNetworkUtility.ini
├── Content/
├── Resources/Icon128.png
├── Source/
│   ├── JWNetworkUtility/
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
│   │       ├── JWNU_HttpRequestJobHandle.cpp
│   │       └── JWNU_BFL_ApiClientService.cpp
│   └── JWNetworkUtilityTest/
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
│   └── openapi.json               (runtime-generated via /docs)
├── Doxygen/                       (gitignored, generated docs)
├── Analysis/                      (gitignored, analysis artifacts)
├── JWNetworkUtility.uplugin
├── CLAUDE.md
├── LICENSE
└── README.md
```
