<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# OpenAI 실시간 전사 — C++ / Blueprint

> 부분 갱신 일자: 2026-09-28 — `UJWNU_Transcriptor` 기반과 공통 OnTranscription 이벤트 추가. 기존 GPT 이벤트는 호환 유지.

공통 결과·경계 계약은 [Scribe 및 공통 계약 가이드](ScribeTranscription.md)를 따른다. GPT는 ExternalCommit에 고정하며 기존 원격 프로토콜은 바뀌지 않는다. 공통/기존 결과 이벤트를 같은 소비자에 중복 바인딩하지 않는다.

> 부분 갱신 일자: 2026-09-27 — 전사 결과에 원본 Delta를 추가하고 전체 Text 기반 UI 연결 계약을 명시.

> 부분 갱신 일자: 2026-09-27 — 캡처 독립 UJWNU_GptLiveTranscriptor를 추가. mono float PCM·누적 부분문·정렬된 최종문·활성 수명 관리 제공.

> 부분 갱신 일자: 2026-09-26 — gpt-realtime-whisper·gpt-live-transcribe 세션, 마이크 컴포넌트와 로컬 검증 추가.

> 부분 갱신 일자: 2026-09-26 — 모델을 enum에서 문자열로 전환. 기본값 `gpt-live-transcribe`, `gpt-transcribe` 추가. 모델별 필드 허용 여부의 클라이언트 거절을 제거하고 서버 판정에 맡긴다.

## 목적과 구조

`JWNetworkUtilityOpenAI` 모듈에서 마이크나 외부 PCM 스트림을 실시간 텍스트로 전사한다. Realtime 전사 세션(`type: "transcription"`)을 쓰는 모델은 이름 문자열로 선택한다. 확인된 모델은 `gpt-live-transcribe`(공식 권장, 기본값), `gpt-transcribe`, `gpt-realtime-whisper`다. 대화 응답과 음성 재생이 필요한 경우는 기존 [GPT-Live](OpenAILive.md)를 사용한다.

`UJWNU_OpenAITranscriptionComponent`는 범용 `UJWNU_MicrophoneCaptureComponent`의 PCM을 장치 독립 `UJWNU_OpenAITranscriptionSession`에 전달한다. 세션은 하위 JWNU WebSocket을 사용하며 `UJWNU_GIS_OpenAI`가 활성 세션의 GC 보관·시간 제한·월드/GameInstance 종료를 관리한다. 별도 모듈·마켓플레이스 의존성·게임 RPC는 추가하지 않는다.

## 캡처 독립 GPT 전사기

`UJWNU_GptLiveTranscriptor`는 외부 PCM을 받아 누적 자막과 입력 순서의 최종문을 제공하는 상위 계층이다. 아래의 원시 세션·마이크 컴포넌트 API도 그대로 사용할 수 있다. ProjectZK·RSR·Silero 의존성은 없으며, 캡처·VAD·게임 명령·키 저장은 호출자가 제공한다.

1. **Create Gpt Live Transcriptor**에 월드 맥락을 전달하고 반환값을 변수에 보관한다.
2. **On Ready**, **On Transcript**, **On Finished**, **On Error**를 바인딩한 뒤 **Start(Options, API Key)** 또는 **Start From Environment(Options)**를 호출한다.
3. **On Ready** 이후 **Append Audio**로 mono float 24kHz PCM을 보낸다. 표본율 변환은 호출자가 맡는다. 구현이 PCM16 LE로 변환해 최대 100ms씩 전송한다.
4. 외부 VAD/버튼에서 **Commit Utterance**를 호출한다. 100ms보다 짧은 발화는 무음 패딩한다. 다음 발화 입력을 계속할 수 있다.
5. **Finish**는 남은 입력을 확정하고 모든 최종문 처리 후 **On Finished**를 알린다. 같은 객체를 다시 Start할 수 있다. **Cancel**은 결과·Finished 알림 없이 즉시 정리한다. 활성 상태 Start는 false와 진단 로그로 거절하며 현재 세션은 유지한다.

C++는 `JWNU_GptLiveTranscriptor.h`를 include하고 `OnReadyNative`, `OnTranscriptNative`, `OnFinishedNative`, `OnErrorNative`에 바인딩한다. 모든 공개 호출·이벤트는 게임 스레드 전용이다. 설정 오류는 Start 안에서 즉시 발생할 수 있다. 환경변수 호출은 기본 공식 Endpoint만 허용하며, Start 자체는 로컬 설정이나 환경 키를 자동으로 찾지 않는다.

`FJWNU_GptLiveTranscript`는 `UtteranceId`, `Text`, `Delta`, `bFinal`을 가진다. **부분문도 누적된 전체 Text**이므로 UI는 해당 발화 자막을 교체한다. Delta는 이번 부분 이벤트의 원본 추가 청크(공백 포함)이며 최종 결과에서는 빈 문자열이다. 추가 청크 연출에 활용할 수 있지만 전체 Text에 다시 더하지 않는다. 최종문은 ack의 PreviousItemId 연속성을 확인하고 입력 순서대로 전달한다. 부분문은 ack 이전에도 즉시 전달한다. 게임 세션 ID와 이전 세션 자막 보관은 호스트 책임이다.

미확정 발화는 30초, 결과 맵은 33개, 발화 텍스트는 16,384자로 제한한다. commit ack/최종문은 제출 후 60초 내에 받아야 한다. Start/Close 제한은 Options를 따른다. 개별 발화 실패도 순서 누락을 피하도록 전체 전사기를 중단하고 OnError를 발생시킨다. 원시 세션의 비치명적 발화 오류 계약과 구분한다.

`UJWNU_GIS_OpenAI`가 활성 객체를 GC로부터 보관하고 실제 시간 기준 deadline을 검사한다. 호출자가 Tick/Poll을 구현할 필요가 없다. 월드/GameInstance 종료는 Cancel하며 완료·취소된 객체는 다음 subsystem Tick에 보관 목록에서 해제한다. 다음 Start에도 쓰려면 BP 변수/UPROPERTY에 보관한다. 콜백에서 Cancel/재시작하면 이전 실행의 후속 BP 이벤트와 대기 최종문은 차단된다. 준비 중 Finish는 취소 후 Finished 한 번, 비활성/종료 대기 중 중복 Finish는 무시한다.

진단 카테고리는 `LogJWNU_GptLiveTranscriptor`다. `GPT.Start/Ready/CommitRequest/CommitAck/Final/Closed`로 단계를 확인하고 `Log LogJWNU_GptLiveTranscriptor Verbose`로 청크·부분문·순서 대기를 확인한다. 로그는 객체·Generation·Item·문자/표본 수를 기록하며 입력 텍스트·PCM·키는 출력하지 않는다. 오류 설명은 공급자 메시지를 포함할 수 있다.

## Blueprint 빠른 시작 — 마이크 컴포넌트

1. Editor 타깃 빌드 후 에디터를 재시작한다.
2. 로컬 플레이어 Actor BP에 **JWNU Open AI Transcription Component**를 추가한다. BeginPlay 자동 연결과 복제는 없다.
3. **On Ready**, **On Transcript**, **On Committed**, **On Error**, **On Closed**를 바인딩한다.
4. **Start From Environment**를 호출한다. `Options`를 연결하지 않으면 `gpt-live-transcribe` 기본값이다. **Make JWNU Open AI Transcription Options**의 `Model`에 모델 이름을 넣는다. **Get Known Transcription Models**가 확인된 이름 목록을 돌려준다. 환경변수 `OPENAI_API_KEY`는 에디터 프로세스가 상속해야 한다.
5. **On Ready** 이후 말한다. 기본 `Use Microphone=true`, `Microphone Device Index=-1`이다.
6. 발화가 끝날 때 **Commit Input Audio**를 호출한다. 버튼 또는 별도 클라이언트 VAD의 종료 판단에서 호출한다. 이 함수는 마이크 캡처를 계속한다.
7. 녹음을 끝낼 때 **Close**를 호출하고 **On Closed**를 기다린다. 다음 입력/다음 Tick의 Start는 새 세션을 만든다.

모든 모델에 `turn_detection=null`을 사용한다. **자동 침묵 감지나 자동 발화 확정은 없다.** 버튼을 눌러 시작하고 떼면서 Close하는 한 번짜리 녹음도 가능하다. 연속 자막에서는 60초 이내마다 발화를 Commit해야 한다. 세션의 `IsActive`, 컴포넌트의 `IsSessionActive`로 활성 여부를 확인한다. 컴포넌트가 상속한 ActorComponent `IsActive`는 전사 상태가 아니다.

`Start`는 직접 전달한 `API Key`를 받는다. 키를 Options·에셋에 넣지 않는다. `StartFromEnvironment`는 아래 기본 공식 URL만 허용한다. 로컬 모의 서버는 `Start`와 빈 키를 사용한다. 제품용 배포 클라이언트의 인증·중계는 호스트가 제공한다.

## 모델과 옵션

| 옵션 | 기본값 / 계약 |
| --- | --- |
| Endpoint | `wss://api.openai.com/v1/realtime?intent=transcription` |
| Model | 모델 이름 문자열. 기본 `gpt-live-transcribe`. 목록 밖 이름도 그대로 전송 |
| Delay | `Default`는 필드 생략. 선택값은 Minimal / Low / Medium / High / XHigh |
| Language | 단일 언어 힌트(`language`). 예: `ko`. 빈 값은 생략 |
| Languages | 언어 힌트 목록(`languages`). 예: `ko`, `en` |
| Prompt | 녹음 상황 문맥(`prompt`). 서버의 길이 제한 적용 |
| Keywords | 용어 힌트(`keywords`). 공백뿐인 값, 개행, `<`, `>` 거절 |
| Start Timeout Seconds | 20초. 연결과 설정 확인까지의 전체 제한 |
| Close Timeout Seconds | 15초. 제출한 모든 최종 결과를 기다리는 제한 |

비어 있지 않은 필드만 전송한다. 클라이언트는 모델 이름·힌트·시간 제한의 **형식**만 검사한다(빈 모델 이름, 공백 포함 이름, 공백뿐이거나 개행·`<`·`>`가 있는 힌트, 알 수 없는 Delay enum, 0 이하·비유한 시간 제한). **모델별 필드 지원 여부는 검사하지 않는다.** 미지원 조합은 서버가 `session.update`를 거절하고 시작 단계의 치명적 OnError(서버 Code·EventId 보존)로 통지된다. 공식 문서끼리도 지원 범위가 어긋나고 수시로 바뀌기 때문이다.

참고용 지원 범위(2026-09-26 GA 클라이언트 이벤트 참조 기준, 서버가 정본):

| 필드 | gpt-live-transcribe | gpt-transcribe | gpt-realtime-whisper |
| --- | --- | --- | --- |
| language | `languages` 사용 권장 | 가능 | 가능 |
| languages · keywords | 가능 | 가능 | 불가 |
| prompt | 가능 | 가능 | GA 세션에서 불가 |
| delay | 전사 가이드는 가능, API 참조는 Whisper 전용으로 기재 | 전사 가이드 기재 | 가능 |

PCM은 **signed PCM16 little-endian, mono, 24kHz**로 고정한다. 범용 마이크 컴포넌트가 장치 표본율을 변환한다. 네트워크 세션만 사용하면 호출자가 이 형식과 실제 음성 속도를 맞춰야 한다.

## 이벤트 처리 — 원시 세션·마이크 컴포넌트

| 이벤트 | 사용법 |
| --- | --- |
| OnReady(SessionId) | `session.updated`의 모델·PCM·수동 발화 설정 확인이 끝난 시점 |
| OnTranscript(Transcript) | `ItemId`, `ContentIndex`, `bFinal`로 구분. 부분은 Delta를 추가, 최종은 Transcript로 해당 발화 전체를 교체 |
| OnCommitted(Commit) | ItemId와 PreviousItemId로 발화 순서를 보관 |
| OnError(Error) | Code, Message, EventId, ItemId, bFatal 확인 |
| OnClosed(Info) | Reason과 bFinalized 확인. 세션당 한 번 |
| OnRawEvent(Type, Json) | 정상 수신 이벤트와 확장 필드 관찰. 최종 이벤트의 usage 등은 원문에서 읽을 수 있음 |

부분 자막은 Commit 이전에도 도착할 수 있다. 발화별 최종 결과 순서는 보장되지 않으므로 최종 문자열을 전역 문자열 뒤에 단순 추가하지 않는다. `PreviousItemId`로 정렬하고 각 ItemId의 최종 문자열을 교체한다. 원시 세션은 자막 누적 맵을 보관하지 않으며 UI/호스트가 필요한 기록을 소유한다. 누적·정렬을 맡기려면 상위 GptLiveTranscriptor를 사용한다. `ClearInputAudio`는 현재 미확정 오디오를 버리므로 대응하는 부분 자막 표시도 호출자가 지운다.

현재 오디오 전용 세션은 content index 0을 받는다. 단어별 타임스탬프·화자 분리·신뢰도 점수·자동 게임 명령 실행은 제공하지 않는다. 원문 이벤트와 자막에 사용자 발화가 포함되므로 필요한 곳에만 저장한다.

## 세션만 사용하는 C++ / BP

호스트 C++ 모듈에 `JWNetworkUtilityOpenAI` 의존성을 추가한다. BP 세션 사용 순서는 **Create OpenAI Transcription Session → 변수 저장 → 이벤트 바인딩 → Start**다. 설정 오류는 Start 호출 안에서 통지될 수 있으므로 먼저 바인딩한다. 세션은 일회용이고 모든 호출·콜백은 게임 스레드 전용이다.

```cpp
#include "JWNU_OpenAITranscriptionSession.h"

// Session은 소유 객체의 UPROPERTY 멤버다.
Session = UJWNU_OpenAITranscriptionSession::CreateOpenAITranscriptionSession(this);
if (Session)
{
    Session->OnTranscriptNative.AddUObject(this, &UMyComponent::HandleTranscript);
    Session->OnErrorNative.AddUObject(this, &UMyComponent::HandleError);
    Session->OnClosedNative.AddUObject(this, &UMyComponent::HandleClosed);
    FJWNU_OpenAITranscriptionOptions Options;
    Options.Model = JWNU::OpenAITranscription::Models::LiveTranscribe; // 기본값. 다른 모델은 이름 문자열
    Options.Languages = {TEXT("ko"), TEXT("en")};
    Session->StartFromEnvironment(Options);
}
```

`OnReadyNative` 이후 `AppendInputAudio`에 외부 PCM을 전달한다. 세션은 마이크를 생성하지 않으므로 서버에서도 사용할 수 있다. 기기 캡처는 로컬 클라이언트가 소유하며 세션·컴포넌트는 복제되지 않는다.

## 수명·종료·제한

상태는 `Idle → Connecting → Starting → Ready → Closing → Closed`이며 치명적 오류는 `Failed`다. WebSocket 연결과 `session.created`만으로 Ready가 되지 않는다. GA `session.update`를 보내고 `session.updated`의 설정을 확인한다. Live API의 `session.start`, 음성 출력, delegation, `session.close`는 보내지 않는다.

| 호출/상황 | 계약 |
| --- | --- |
| AppendInputAudio | Ready에서만. 비어 있지 않은 짝수 바이트, 청크당 최대 4,800바이트(100ms). 미확정 입력 최대 60초 |
| CommitInputAudio | Ready, 최소 4,800바이트(100ms), 미완료 발화 32개 미만일 때 접수 |
| ClearInputAudio | 미확정 입력을 비운다. 기존 제출 발화의 최종 결과는 계속 수신 |
| Close | 미전송 장치 캡처를 중단하고 이미 세션에 전달된 입력을 Commit. 100ms 미만 꼬리는 무음 패딩. 모든 commit ack와 최종 전사/실패를 기다린 후 소켓 종료 |
| 시작 중 Close / Cancel | 즉시 취소, bFinalized=false. 최종 결과 보장 없음 |
| 서버 명령 오류 | 버퍼 상태 불일치를 피하도록 치명적 종료, 오류 event_id 보존 |
| 개별 전사 실패 | ItemId가 있는 비치명적 OnError. 나머지 발화는 계속 처리하며 종료 bFinalized=false |
| 월드/GameInstance/컴포넌트 종료 | 세션과 캡처 즉시 취소 |

`bFinalized=true`는 모든 제출 발화가 성공적으로 끝난 정상 Close를 뜻하며 서버의 별도 session.closed 수신을 뜻하지 않는다. 입력 없이 Close한 경우도 true다. 캡처 컴포넌트의 게임 스레드 전달 이전에 남은 짧은 꼬리 버퍼는 StopCapture에서 버려질 수 있다. 마지막 소리를 보존해야 하면 녹음을 충분히 마친 뒤 Close한다.

송신 큐는 128KiB로 제한하며 거절되면 세션이 실패한다. 공개 입력 함수의 false는 상태/크기/대기 상한 위반 또는 전송 실패다. 마이크 편의 컴포넌트는 입력 거절 시 캡처와 세션을 중단한다. 자동 재연결·재전송은 없다.

## 로컬 검증

```powershell
cd Plugins/JWNetworkUtility/TestServer
uv sync --locked
uv run uvicorn main:app --host 127.0.0.1 --port 5000 --reload
```

Options.Endpoint를 `ws://127.0.0.1:5000/realtime?intent=transcription`로 설정하고 Start에 빈 키를 전달한다. 고정 한국어 부분/최종 자막이 반환되며 실제 음성 인식·과금은 없다. `Use Microphone=false`이면 OnReady 이후 GetSession으로 합성 PCM을 보낼 수 있다. 24kHz 100ms 무음은 4,800바이트의 0 배열이다.

프로젝트 루트의 자동 테스트 명령:

```powershell
uv run --locked --project Plugins/JWNetworkUtility/TestServer python Plugins/JWNetworkUtility/TestServer/run_transcription_tests.py --engine "C:/Program Files/Epic Games/UE_5.7" --project "C:/Users/prays/Documents/Unreal Projects/ProjectZK/ProjectZK.uproject"
```

실행기는 로컬 FastAPI와 별도 UnrealEditor-Cmd의 수명을 관리한다. `JWNetworkUtility.OpenAI.Transcription.Codec`는 필드 전송·모델 이름 통과·형식 검증을, `.FastAPI`는 세 모델의 실제 WebSocket·서버 측 모델별 필드 거절·컴파일한 BP·GC·월드/컴포넌트/GI 정리·설정/타임아웃/오류·최종 결과 순서·콜백 중 Close를 검증한다. 추가 `.GptLiveTranscriptor.StreamingResults/Lifetime/FastAPI`는 PCM·부분문·최종문 정렬·시간 제한·콜백 중 취소와 독립 BP 호환 이벤트·GC 보관·동일 객체 재시작을 검증한다. 전사 실행기는 총 5개 테스트를 실행한다. 보고서는 호스트 `Saved/Automation/JWNUOpenAITranscription-<timestamp>`에 기록된다. 실패 fixture는 `JWNU_SSE_TEST_FIXTURES=1`에서만 활성화한다.

실제 계정 접근 권한·두 모델의 실제 인식 품질·물리 마이크·Server 타깃 빌드와 패키징은 이 모의 테스트가 검증하지 않는다.

2026-09-26 검증: ProjectZKEditor·ProjectZK Win64 Development 빌드 통과. Codec·FastAPI 2종과 통합 21개 시나리오 통과(`Saved/Automation/JWNUOpenAITranscription-20260926-175111/index.json`). 기존 GPT-Live 회귀도 통과(`Saved/Automation/JWNUOpenAILive-20260926-175424/index.json`). 두 실행에서 기존 DefaultJWNetworkUtility.ini 경로 정규화 경고 1건이 각각 기록되었고 실패는 없다.

2026-09-26 모델 문자열 전환 후 재검증: ProjectZKEditor·ProjectZK 빌드 통과. Codec과 통합 21개 시나리오 통과(`Saved/Automation/JWNUOpenAITranscription-20260926-195845/index.json`). 시나리오 1·2·18은 각각 gpt-live-transcribe·gpt-transcribe 정상 흐름과 Whisper+prompt의 서버 거절을 검증한다. 동일한 경로 정규화 경고 1건 외 실패는 없다.

2026-09-27 GPT 전사기 이관 검증: 호스트 Editor·Game Development 빌드 성공. 전사 5종(기존 Codec·FastAPI 및 새 StreamingResults·Lifetime·FastAPI)과 호스트 회귀 9종, 총 14개 통과. 플러그인 단독 WebSocket 경로의 GC 보관·동적 이벤트·정상 종료 후 재시작·콜백 중 취소까지 확인했다. 호스트 보고서: `Saved/Automation/SpeechPluginRefactor/index.json`. 3개 테스트에 기존 설정 경로 정규화 경고가 있으며 실패는 없다. 실제 마이크·유료 API는 호출하지 않았다.

## 구현과 확장

- 누적·정렬 API: `Public/JWNU_GptLiveTranscriptor.h`, 결과 타입 `JWNU_GptLiveTranscriptorTypes.h`, 구현 `Private/JWNU_GptLiveTranscriptor.cpp`.
- 공개 API: `Source/JWNetworkUtilityOpenAI/Public/JWNU_OpenAITranscriptionTypes.h`, `JWNU_OpenAITranscriptionSession.h`, `JWNU_OpenAITranscriptionComponent.h`.
- 형식 검증·송신 스키마: `Private/JWNU_OpenAITranscriptionJson.h`. 확인된 모델 이름: `JWNU::OpenAITranscription::Models`(Types 헤더).
- 수명·입력: `Private/JWNU_OpenAITranscriptionSession.cpp`, 수신: `JWNU_OpenAITranscriptionSession_Events.cpp`.
- 모의 프로토콜: `TestServer/transcription_examples.py`. 공급자 코드는 OpenAI 모듈 안에 유지한다.

같은 전사 세션 프로토콜을 쓰는 새 모델은 코드 변경 없이 `Model` 이름만 바꿔 사용한다. 확인한 모델은 `Models` 상수·`GetKnownTranscriptionModels`·모의 서버 `MODEL_UNSUPPORTED_FIELDS`에 추가한다. 폐기된 모델은 목록에서 빼기만 한다. 새 필드가 생기면 Options·직렬화·Codec 테스트를 함께 확장한다. 이벤트·세션 프로토콜이 다른 공급자(Gemini Live, ElevenLabs 등)는 이 세션에 섞지 않고 별도 모듈·세션으로 구현한다. 실제 모델 사양 변경은 아래 공식 문서를 먼저 확인한다.

공식 근거: [Realtime 전사 가이드](https://developers.openai.com/api/docs/guides/realtime-transcription), [GA 클라이언트 이벤트](https://developers.openai.com/api/reference/resources/realtime/client-events), [Whisper 모델](https://developers.openai.com/api/docs/models/gpt-realtime-whisper), [Live Transcribe 모델](https://developers.openai.com/api/docs/models/gpt-live-transcribe). 확인일 2026-09-26. 필드 기본값은 생략하여 서버 기본 설정을 사용한다.
