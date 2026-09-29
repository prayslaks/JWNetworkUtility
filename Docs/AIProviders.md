<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# AI 공급자 키 관리

> 부분 갱신 일자: 2026-09-30 — JWCU에서 독립한 JWNU 공급자 설정·GIS·비공개 암호화 저장소.

## 빠른 시작

1. 빌드 후 에디터를 재시작한다. **Project Settings → JWNetworkUtility → AI Provider Settings**에서 공급자를 선택한다.
2. **+ 키 추가**에 이름·키·선택적 관리용 만료일(YYYY-MM-DD, UTC)을 입력하고 **암호화하여 저장**한다. 첫 키는 자동 선택하며 이후에는 **사용 키로 선택**한다.
3. BP의 **Get Game Instance Subsystem → JWNU AI Provider Subsystem**에서 `GetApiKey("OpenRouter")`처럼 조회한다. 성공한 경우에만 반환값을 공식 endpoint 요청에 명시적으로 전달한다. [System One 연결](SystemOne.md).
4. C++ 소비 모듈은 `JWNetworkUtilityAI`를 Build.cs 의존성에 추가하고 `JWNU_AIProviderSubsystem.h`를 포함한다. GameInstance가 없는 에디터 네이티브 호출자는 `UJWNU_AIProviderSettings::GetApiKey`를 사용할 수 있다.

`OpenAI`·`Gemini`·`ElevenLabs`·`OpenRouter`는 코드에 등록된 키 발급 서비스 ID다. 모델 ID나 응답의 실제 처리 공급자와 구분한다. 등록은 해당 공급자의 모든 모델 API가 구현됐다는 의미가 아니다. 카탈로그는 `GetProviderCatalog`에서 관리하며 지원 종료 항목은 `bDeprecated`로 남겨 목록 조회·삭제만 허용한다.

## 소유권과 공개 경계

| 모듈 / 타입 | 역할 |
| --- | --- |
| `JWNetworkUtilityAI` / `UJWNU_AIProviderSettings` | 카탈로그·프로젝트별 설정 진입점 |
| `UJWNU_AIProviderSubsystem` | BP 키 관리·조회와 GameInstance별 임시 키 |
| `FJWNU_APIKeyInfo` | ID·이름·마스킹·선택 상태·등록/수정/관리용 만료일; 비밀 원문 없음 |
| `JWNetworkUtilityEditor` | 공급자 카드·키 추가/수정·선택·삭제 UI |
| AI 모듈 Private의 `FJWNU_APIKeyVault → FJWNU_APIKeyStore → FJWNU_KeyCrypto` | 다중 키 직렬화·검증 파일 교체·Windows DPAPI |

저장소와 암호화는 UCLASS/UFUNCTION·모듈 export가 없는 비공개 구현이다. 외부 모듈에서 헤더를 포함하거나 범용 암호화 API로 사용하지 않는다. JWCU 모듈 의존성은 없다. JWCU의 범용 PlatformCrypto는 별도 기능으로 남는다. 키 관리 모듈에는 모델별 요청·게임 타입 의존성이 없다.

## 키 관리 계약

| API | 동작 |
| --- | --- |
| `ListKeys` | 비밀 없는 목록. 8자 초과 키만 끝 4자리 표시 |
| `SaveKey` | 빈 Id는 신규 등록. 기존 Id 편집 시 빈 ApiKey는 기존 비밀 유지. 첫 키만 자동 선택 |
| `SelectKey` | 저장 사용 키 선택. 다음 조회부터 적용 |
| `DeleteKey / DeleteAllKeys` | 로컬 저장 키 제거. 사용 키 삭제 후 다른 키를 자동 선택하지 않음 |
| `GetApiKey / HasApiKey` | 현재 GI 임시 키 우선, 없으면 선택한 저장 키 복호화. 실패하면 빈 출력 |
| `SetRuntimeApiKey` | 현재 GI에만 임시 적용. 빈 값은 저장 키 조회도 차단 |
| `ClearRuntimeApiKey` | 임시 키를 비우고 저장 키 사용으로 복귀 |

저장 목록·선택은 같은 프로젝트에서 공유하며 임시 키는 GI별로 격리한다. GIS의 선택·사용 키 편집·사용 키 삭제는 호출한 GI의 임시 키만 해제한다. 에디터 선택 변경은 GI의 임시 키를 해제하지 않는다. Deinitialize/BeginDestroy에서 임시 키 버퍼를 지운다. 진행 중인 요청의 인증은 선택 변경으로 바뀌지 않는다.

공급자당 최대 32개, 이름 64자, 키 16,384 TCHAR, 전체 직렬화 목록 131,072 TCHAR다. 관리용 만료일은 UTC 표시 정보이며 원격 키 유효성·권한·quota를 조회하거나 API 사용을 차단하지 않는다. 삭제도 공급자 서버의 키를 폐기하지 않는다. 손상된 저장본은 추가·편집으로 덮어쓰지 않는다. `DeleteAllKeys`는 명시적 전체 초기화이므로 빈 목록으로 교체할 수 있다.

## 암호화와 수명

저장 경로는 `FPlatformProcess::UserSettingsDir()/JWNetworkUtility/APIKeys/<식별자 해시>.bin`이다. AppId는 `FApp::GetProjectName()`, KeyName은 `Vault-<Provider>`다. 프로젝트 이름은 ASCII 영숫자·하이픈·밑줄 1~128자여야 하며 이름 변경 시 별도 저장소가 된다. `JWNU.APIKey.v1/<AppId>/<KeyName>` Context를 DPAPI 추가 엔트로피로 사용한다. 경로 해시는 식별용이고 암호화는 현재 Windows 사용자 계정의 DPAPI가 수행한다.

고유 임시 파일에 암호문을 저장한 뒤 다시 읽고 복호화해 원문과 비교한다. 성공한 파일만 `MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH)`로 교체한다. 실패 시 기존 저장본을 보존하고 임시 파일은 제거한다. 지원하지 않는 플랫폼에서 평문으로 대체하지 않는다.

키·메타데이터는 Config·에셋에 저장하지 않는다. 내부 임시 버퍼는 사용 후 덮어쓰지만 엔진·문자열 변환기·호출자의 모든 복사본까지 지우지는 못한다. 반환받은 비밀값은 로그·Print String·SaveGame·BP 기본값에 연결하지 않는다. 같은 Windows 계정 권한의 프로세스 사이를 격리하는 저장소는 아니다.

## 호환 제거

[Deprecated 2026-09-30] JWCU APIKeyStore/APIKeyVault BFL·AIProviderSettings/Subsystem 및 ProjectZK의 AIProviderAPIKeysSettings/SpeechLocalSettings 호환 경로.

옛 UClass·BP 노드·구조체 redirect는 제공하지 않는다. 기존 BP는 `JWNU AI Provider Subsystem`과 `FJWNU_APIKeyInfo`로 교체해야 한다. 이전 JWCU 암호문·단일 키·평문 ini를 자동으로 읽거나 이관하지 않는다. 기존 파일을 삭제하지 않으며 새 설정 화면에서 키를 다시 등록한다. 프로세스 전역 임시 키 API도 제거했다.

## 검증

`JWNetworkUtility.AI` 자동 테스트는 DPAPI 이진 왕복·Context 불일치·변조 거절, UTF8 저장·평문 미포함·읽기 전용 파일 교체 실패 시 원본 보존·손상 거절, 다중 키 선택·마스킹·편집·삭제, GI별 override 격리, UI 날짜 입력을 검사한다. 가짜 키와 고유 테스트 AppId를 사용하며 실제 사용자 키·외부 API에 의존하지 않는다.

2026-09-30: ProjectZKEditor/Game Win64 Development 빌드 성공. JWNU AI 5종, JWCU 범용 암호화 1종, ProjectZK 음성 7종 총 13개 통과(음성 1종 경고 포함). 보고서: 호스트 `Saved/Automation/JWNUAIProviders/index.json`. 실제 API 인증과 설정 화면 육안 검수는 별도다.

## 구현 위치

- [공급자 설정](../Source/JWNetworkUtilityAI/Public/JWNU_AIProviderSettings.h)
- [GameInstance API](../Source/JWNetworkUtilityAI/Public/JWNU_AIProviderSubsystem.h)
- [공개 메타데이터](../Source/JWNetworkUtilityAI/Public/JWNU_AIProviderTypes.h)
- [비공개 저장소](../Source/JWNetworkUtilityAI/Private/JWNU_APIKeyVault.h)
- [에디터 UI](../Source/JWNetworkUtilityEditor/Private/JWNU_AIProviderKeysCustomization.cpp)
