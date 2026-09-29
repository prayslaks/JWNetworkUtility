<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# System One — 공통 판단 요청

> 부분 갱신 일자: 2026-09-30 — TypeSafe 전용 모듈·공개 타입·호환 계층을 제거하고 System One API와 테스트로 통합.

> 부분 갱신 일자: 2026-09-30 — JWNU가 공급자 키 관리·내부 암호화를 독립 소유하도록 이관. 기존 키 API·자동 이관 제거.

> 부분 갱신 일자: 2026-09-30 — 공통 Runtime 모듈, OpenRouter 경로·모델 프리셋, TypeSafe 호환 연결과 로컬 검증 추가.

## 목적과 경계

`JWNetworkUtilitySystemOne`은 상태와 질문을 보내 Choice·Score·Noul 판단을 받는 C++/Blueprint 모듈이다. 모델마다 클래스를 만들지 않고 같은 `UJWNU_SystemOneRequest`에 모델 문자열을 전달한다. OpenRouter 경유 Jev·Span-01·Solar Decide를 위한 프리셋을 제공하며, 다른 System One 모델 ID도 입력할 수 있다.

JWNU는 HTTP·인증 헤더·JSON·요청 수명을 소유한다. 키 영구 저장은 같은 플러그인의 선택적 JWNetworkUtilityAI 모듈이 소유한다. SystemOne 모듈은 저장소를 자동 조회하지 않고 명시적 키를 받으며 JWCU 의존성이 없다. 게임 상태 수집, 실행 임계값, 서버 권한 검사, 오래된 응답 폐기와 실제 명령 실행은 호출자 책임이다. 음성 입력·Transcriptor 상속·모델별 ProjectZK 어댑터는 필요 없다.

공통화 대상은 OpenRouter의 정규화된 API와 호환 System One API다. Respan 등의 직접 API까지 같은 양식이라고 가정하지 않는다. 실제 모델 접근 권한·한국어 정확도·서비스 지연은 모의 테스트로 검증할 수 없다.

## Blueprint 빠른 시작

1. 빌드 후 에디터를 재시작한다. 새 Runtime 모듈이 로드되어야 한다.
2. 저장 키를 사용할 경우 **Project Settings → JWNetworkUtility → AI Provider Settings → OpenRouter**에서 키를 추가하고 사용 키로 선택한다.
3. 현재 GameInstance의 **JWNU AI Provider Subsystem → GetApiKey**에 `OpenRouter`를 전달한다. 반환 bool이 false면 호출을 중단한다. 키는 변수 기본값·에셋·ini·로그에 보관하지 않는다.
4. `Make JWNU System One State`에 Text 또는 Json 형식과 상태를 넣는다.
5. `Make Choice Question`, `Make Score Question`, `Make Noul Question`으로 질문 배열을 만든다. **JWNU → SystemOne** 카테고리의 노드를 사용한다. ID는 결과 조회용이며 실제 판단 지침은 Instructions에 쓴다.
6. `Make Open Router Options`에서 모델 문자열을 선택한다. `Get Open Router Model Presets`는 선택용 목록일 뿐 허용 목록이 아니다. 저장 키를 전달하는 이 경로에서는 프리셋의 공식 Endpoint를 유지한다.
7. **Create System One Request → 변수 보관 → OnCompleted / OnFailed 바인딩 → Start** 순서로 호출한다. Start의 API Key에 3번 출력을 전달한다.
8. 성공 Result의 Choices·Scores·Nouls 맵을 질문 ID로 조회한다. 취소는 Request의 Cancel이다.

`Call System One API`는 State·Questions·Options·ApiKey·성공/실패 콜백을 받아 즉시 예약하는 편의 노드다. 반환 요청은 취소·조회가 필요할 때 보관한다. 반환 요청에 Start를 다시 호출하지 않는다. Options와 콜백은 미연결 기본값을 지원한다.

키 관리와 호환 제거 범위는 [AIProviders.md](AIProviders.md)를 따른다. 키 저장소를 거치지 않고 직접 키를 인자로 전달하거나 `Start From Environment` / `Call System One API From Environment`를 사용할 수도 있다. SystemOne 요청은 JWNU AI 저장 키를 자동으로 읽지 않는다.

## 연결과 모델 설정

| 설정 | 기본값·의미 |
| --- | --- |
| Endpoint | `https://openrouter.ai/api/alpha/decisions` |
| Model | `~typesafe/jev-latest` |
| MakeOpenRouterOptions(Model) | 공식 Decisions 주소와 지정한 모델 |
| MakeTypeSafeOptions(Model) | `https://api.typesafe.ai/v1/systemone`, 기본 모델 `jev-latest` |
| OpenRouter 모델 프리셋 | `~typesafe/jev-latest`, `respan/span-01`, `upstage/solar-decide` |
| TimeoutSeconds / AttemptTimeoutSeconds | 전체 20초 / 시도당 10초 |
| MaxRetries | 최초 전송 이후 2회 |
| InitialRetrySeconds / MaxRetrySeconds | 0.25초 / 4초; Retry-After가 더 길면 우선 |
| MaxResponseBytes | 기본 2 MiB, 허용 1 byte~16 MiB |

OpenRouter의 TypeSafe SDK 호환 경로인 `https://openrouter.ai/api/v1/systemone`도 Endpoint에 명시적으로 지정할 수 있다. 경로 실패 시 다른 경로로 자동 전환하지 않는다. Model은 입력한 문자열 그대로 전송하고, 실제 응답한 모델 ID는 Result.Model에 보존한다. 이동 별칭의 판단 결과가 바뀌면 임계값을 재검증하거나 모델 버전을 고정한다.

환경변수 키는 정확한 공식 URL 세 개만 허용한다. OpenRouter의 두 URL은 `OPENROUTER_API_KEY`, TypeSafe 직접 URL은 `TYPESAFE_API_KEY`를 읽는다. query·다른 호스트·다른 경로에는 환경변수 키를 자동 전달하지 않는다. 명시적 ApiKey 경로는 호출자가 정한 HTTPS endpoint를 허용하므로 저장 키를 조회하는 호출부에서 주소를 먼저 검증해야 한다. 평문 HTTP는 **키 없는** `127.0.0.1:port` 또는 `localhost:port` 모의 서버만 허용한다.

JWNU의 `OpenRouter`는 키 발급 서비스 ID다. `Result.Provider`는 실제 처리 공급자이며 키 조회 ID로 사용하지 않는다. TypeSafe 직접 키를 JWNU의 OpenRouter 항목에 넣지 않는다. 현재 JWNU 카탈로그에 TypeSafe 직접 키 저장 항목은 추가하지 않았다.

## 질문·결과 계약

요청은 `model`, `state`, `questions`다. Text 상태는 JSON처럼 보여도 문자열로 전송한다. Json 상태는 문자열·객체·배열을 허용하며 숫자·bool·null은 거절한다. Questions는 비어 있지 않고 질문 ID가 중복되지 않아야 한다.

| 질문 | Criteria | 응답 |
| --- | --- | --- |
| Choice | 1~255개 선택지 ID → 설명 맵 | Choice, Probabilities, Confidence |
| Score | 낮은 단계부터 2~10개 설명 배열 | 실수 Score, Legend, Probabilities, Confidence |
| Noul | 선택적 TrueDescription / FalseDescription | 0~1 확률 Noul; bool로 변환하지 않음 |

`InstructionsJson`·`CriteriaJson`은 고급 JSON 입력이며 비어 있지 않으면 일반 필드보다 우선한다. Noul 설명을 모두 비우면 criteria를 생략한다. 요청 UTF-8 본문 상한은 2 MiB이며 모델의 토큰 한도와는 별개다. 현재 기준 개수 제한은 기존 Jev 호환 계약을 유지한다. 모델별 차이가 확인되면 공통 스키마와 모델 제한을 구분해 확장한다.

응답 질문 ID 집합·타입은 요청과 일치해야 한다. 확률·confidence는 유한한 0~1 숫자, 분포 합은 1에서 0.002 이내여야 한다. Choice는 요청한 선택지 중 최대 확률의 ID여야 한다. Score는 단계 인덱스 범위의 실수를 유지한다. 필수 사용량 input_tokens·output_tokens는 음수가 아닌 정수다. 오류가 있으면 부분 결과를 노출하지 않는다.

| 추가 결과 | 의미 |
| --- | --- |
| RequestId | 응답 `id`, 미제공 시 빈 문자열 |
| Provider | 응답 `provider`, 미제공 시 빈 문자열 |
| bHasCost / Cost | `usage.cost`의 존재 여부와 USD 비용. 0은 제공된 값, 누락·null은 미제공 |
| Attempts | 최초 전송을 포함한 로컬 HTTP 시도 횟수 |

명시적으로 제공한 추가 메타데이터의 타입도 검증한다. 그 외 알 수 없는 필드는 무시한다. confidence와 Noul은 정확도 보증이 아니며 임계값은 호출자가 검증한다. 여러 질문은 같은 state를 독립적으로 평가한다.

## 수명과 오류

공개 호출은 게임 스레드 전용이고 Request는 일회용이다. Create는 네트워크를 시작하지 않는다. Start 전 프레임을 넘길 때는 BP 변수 또는 C++ UPROPERTY로 요청을 보관한다. Start부터 `UJWNU_GIS_SystemOne`이 GC 보호·실시간 Pump·월드/GameInstance 종료 취소를 관리한다.

정상 월드의 최초 Start에서 입력 오류가 나면 false와 함께 오류 이벤트를 다음 Pump에 예약한다. 그동안 IsActive는 true다. 재사용 요청이나 종료 중인 월드에서는 예약 없이 false다. 타입별 이벤트 이전에 최종 상태를 저장하고, 네이티브 이벤트 → BP 이벤트 → OnFinished를 한 번 전달한다. 취소는 OnFailed(Cancelled)다. [공통 수명 계약](RequestLifecycle.md).

전송은 기존 `UJWNU_JsonHttpJob`을 공유한다. 네트워크 오류·시도 제한·429·500·502·503·504·529를 예산 내 재시도한다. 401·402·422와 응답 스키마 오류는 재시도하지 않는다. 재시도는 원격 중복 평가·추가 비용을 발생시킬 수 있으므로 필요한 경우 MaxRetries를 0으로 지정한다. HTTP 오류 본문은 Error.ResponseBody에 보존하며 키·본문을 자동 로그에 쓰지 않는다.

## TypeSafe 통합과 확장

공개 요청·구조체·Codec·GIS·BP 함수는 모두 `SystemOne`으로 통일한다. [Deprecated 2026-09-30] `JWNetworkUtilityTypeSafe`와 `FJWNU_TypeSafe*`·`UJWNU_TypeSafeRequest`·전용 BFL/GIS 및 핀 redirect를 제거했다. 호환 클래스·변환 계층을 남기지 않는다. 저장된 옛 BP 에셋은 자동 변환하지 않는다.

| 기존 사용처 | 통합 후 |
| --- | --- |
| Build.cs의 `JWNetworkUtilityTypeSafe` | `JWNetworkUtilitySystemOne` |
| `FJWNU_TypeSafe*` | 대응하는 `FJWNU_SystemOne*` |
| Create TypeSafe Request | Create System One Request |
| Call TypeSafe API / From Environment | Call System One API / From Environment |
| TypeSafe 기본 옵션 | 기본값은 OpenRouter. TypeSafe 직접 호출 시 **Make Type Safe Options**를 명시적으로 연결 |

TypeSafe 서비스 직접 호출은 유지한다. `MakeTypeSafeOptions()`의 endpoint는 `https://api.typesafe.ai/v1/systemone`, 모델은 `jev-latest`다. 이를 공통 요청의 Options에 연결하고 TypeSafe 키를 명시적으로 전달하거나 환경변수 방식으로 `TYPESAFE_API_KEY`를 사용한다. OpenRouter 키와 섞지 않는다.

새 모델은 Model 문자열만 추가하면 된다. 프로토콜이 같은 모델에 클래스를 추가하지 않는다. 신규 결과 메타데이터는 공통 Result·Codec·테스트를 함께 갱신한다. 공통 계층에는 게임 명령·RPC·임계값을 넣지 않는다.

| 구현 | 파일 |
| --- | --- |
| 공개 요청 | [JWNU_SystemOneRequest.h](../Source/JWNetworkUtilitySystemOne/Public/JWNU_SystemOneRequest.h) |
| 데이터 계약 | [JWNU_SystemOneTypes.h](../Source/JWNetworkUtilitySystemOne/Public/JWNU_SystemOneTypes.h) |
| BP 편의 함수 | [JWNU_BFL_SystemOne.h](../Source/JWNetworkUtilitySystemOne/Public/JWNU_BFL_SystemOne.h) |
| JSON 변환·검증 | [JWNU_SystemOneCodec.cpp](../Source/JWNetworkUtilitySystemOne/Private/JWNU_SystemOneCodec.cpp) |
| 활성 요청 소유 | [JWNU_GIS_SystemOne.cpp](../Source/JWNetworkUtilitySystemOne/Private/JWNU_GIS_SystemOne.cpp) |

C++ 소비 모듈은 Build.cs에 `JWNetworkUtilitySystemOne`을 추가한다. 공개 헤더에서 공통 타입을 노출할 경우 PublicDependency로 둔다. ProjectZK에 모델별 어댑터나 새 런타임 사용처를 자동으로 추가하지 않는다.

## 검증과 문제 해결

```powershell
uv run --locked --project Plugins/JWNetworkUtility/TestServer python Plugins/JWNetworkUtility/TestServer/run_systemone_tests.py --engine "C:/Program Files/Epic Games/UE_5.7" --project ProjectZK.uproject
```

SystemOne.Codec는 세 질문 계약과 모델 문자열, 선택적 비용·추가 메타데이터를 검증한다. FastAPI·Immediate는 각 8개 시나리오로 세 모델 ID 전송·한글 상태·실제 BP 컴파일과 결과 전달·기본 Options·환경변수 주소 제한·취소·GC·월드/GI 종료를 검증한다. Transport.FastAPI·Transport.Immediate는 각각 기존 25개 시나리오를 공통 API에서 직접 검증한다. 429/529·Retry-After·시도 상한·타임아웃·수신 상한·입력 오류 지연 전달·GC·종료와 BP Options/콜백 기본값을 포함한다. 전체 실행은 Codec 포함 5개 자동 테스트다. JWNetworkUtility.AI.AIProviderSubsystem은 가짜 OpenRouter 키와 고유 AppId로 영구 선택·GI 격리를 검증한다.

로컬 `/systemone/decisions`와 `/systemone/direct`는 실제 모델이 아니라 고정 응답이다. 보고서는 호스트 Saved/Automation 아래 생성된다. 실제 API 인증·모델별 계약·한국어 판단 품질은 별도 확인 대상이다. 새 노드가 보이지 않으면 빌드 후 에디터를 재시작한다. Credentials는 키 조회 결과·공식 URL을, InvalidResponse는 반환된 계약과 모델 지원을, Http는 상태 코드·오류 본문을 확인한다.

공식 계약 확인일: 2026-09-29. [OpenRouter Decisions](https://openrouter.ai/docs/api/api-reference/alphadecisions/submit-a-decisions-request), [TypeSafe SDK 호환 경로](https://openrouter.ai/docs/guides/community/typesafe-sdk).

2026-09-30 검증: UE 5.7 Win64 Development Editor·Game 빌드 성공. SystemOne 3종(`JWNUSystemOne-20260930-002212`), TypeSafe 3종(`JWNUTypeSafe-20260930-001854`), JWCU Security 4종(`JWCUOpenRouter`), 총 10개 통과·실패 0. SystemOne/TypeSafe 보고서에는 기존 INI 경로 정규화 경고가 포함되며 TypeSafe의 수신 상한 시나리오는 의도적인 libcurl 수신 중단 경고를 남긴다. Python 문법·문서 링크도 확인했다. 실제 외부 API와 저장된 사용자 BP 에셋의 수동 실행·설정 UI 육안 검수는 수행하지 않았다.

2026-09-30 통합 후 검증: TypeSafe 모듈·노드 제거 상태로 Editor/Game 빌드 성공. `JWNUSystemOne-20260930-012418`의 5개 테스트와 `JWNUSystemOneCommon`의 공통 요청·BP 노드 2개가 모두 통과했다. 기존 25개 전송 시나리오를 Create/Start와 즉시 호출 각각에서 실행했으며, OpenRouter 테스트는 각 8개 시나리오를 유지한다. INI 경로 정규화 및 의도적 수신 상한 초과에 따른 libcurl 경고가 포함되며 실패는 0이다. 실제 외부 API·저장된 사용자 BP 에셋 실행은 검증하지 않았다.
