<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# Scribe V2 Realtime와 공통 전사 계약

> 부분 갱신 일자: 2026-09-28 — 공급자 중립 결과·실행 계약, Scribe 서버 VAD 전사기 및 로컬 소켓 검증 추가.

## 모듈과 소유권

`JWNetworkUtilityAudio`의 `UJWNU_Transcriptor`는 외부 mono float PCM을 받는 게임 스레드 실행 계약이다. `GetBoundaryMode`, `GetSampleRate`, `AppendAudio`, `CommitUtterance`, `Finish`, `Cancel`을 공유한다. BP에서는 `GetTranscriptBoundaryMode`, `RequestCommit`과 공통 이벤트를 사용한다. `RequestCommit`은 공급자 관리 모드에서 false를 반환한다. typed Start 옵션과 키 전달은 각 공급자가 소유한다.

`FJWNU_TranscriptUpdate`는 세션 내 `SegmentId`, 현재 전체 `Text`, 선택적인 원본 `Delta`, `bFinal`을 전달한다. ID는 불투명 값이며 다른 Start 사이에는 비교하지 않는다. 공급자와 관계없이 UI는 Text 전체로 교체한다. 공통 이벤트는 `OnTranscriptionReady/Update/Finished/Error` 및 같은 이름의 Native delegate다.

GPT는 `ExternalCommit`으로 동작하며 기존 `OnReady/Transcript/Finished/Error`와 `FJWNU_GptLiveTranscript`도 호환 전달한다. 같은 소비자는 공통 또는 기존 이벤트 중 하나만 구독한다. 기존 GPT subsystem 수명 관리는 유지된다.

`JWNetworkUtilityElevenLabs`의 `UJWNU_ScribeTranscriptor`는 core WebSocket 및 Audio 계약에 의존하며 호스트 프로젝트·JWCU·OpenAI 모듈에 의존하지 않는다. **호출자는 반환 UObject를 UPROPERTY/BP 변수로 보관해야 한다.** Tick에서 시작/종료 제한과 월드 종료를 확인하며, Cancel/BeginDestroy는 연결을 해제한다.

## Scribe 사용 순서

1. `CreateScribeTranscriptor(WorldContext)` 결과를 변수에 보관한다.
2. 공통 Ready/Update/Finished/Error를 바인딩하고 `Start(FJWNU_ScribeOptions, ApiKey)`를 호출한다.
3. Ready 이후 mono float **16kHz, 무음 포함** PCM을 AppendAudio에 전달한다. 로컬 VAD로 무음 청크를 제거하지 않는다.
4. 공급자 관리 모드이므로 외부 Commit은 하지 않는다. 일반 발화는 서버가 확정한다.
5. 녹음을 끝낼 때 캡처의 마지막 PCM을 전달한 뒤 Finish한다. 즉시 중단은 Cancel이다.

모델 `scribe_v2_realtime`, `commit_strategy=vad`, PCM16 LE 16kHz, 서버 무음 경계 0.7초, 최소 speech/silence 100ms는 구현에 고정한다. 호스트는 공급자 이름에 따라 분기하지 않고 `ProviderManaged` 정책만 읽는다. 사용자 옵션은 endpoint, 언어 코드, 시작 timeout, Finish 결과 수신 시간이다.

공식 endpoint는 `wss://api.elevenlabs.io/v1/speech-to-text/realtime`이며 키는 `xi-api-key` header로 전달한다. 키 없는 loopback `ws://127.0.0.1:port`/`ws://localhost:port`는 테스트용이다. endpoint의 query/fragment는 거절하여 구현에 고정한 정책을 덮어쓰지 못하게 한다. 키 저장/선택은 호스트가 제공한다. [공식 API](https://elevenlabs.io/docs/api-reference/speech-to-text/v-1-speech-to-text-realtime)

## 결과와 종료의 의미

- `session_started`의 모델·PCM 규격을 검증한 뒤 Ready를 방송한다.
- `partial_transcript`는 이전 문장을 교체하며 Delta를 합성하지 않는다. `committed_transcript`에서 현재 ID를 닫고 다음 ID로 진행한다. 서버 메시지 스트림의 구간 순서를 사용하며 GPT의 ACK나 원격 item ID를 요구하지 않는다.
- timestamps/entities/edited 결과는 명령 Final로 재방송하지 않는다. 같은 문장의 다음 발화는 별도 ID로 정상 전달한다.
- 100ms 단위로 전송하며 tail은 그 미만으로 제한된다. 단일 AppendAudio 호출은 최대 10초, 전송 큐는 256KiB, 텍스트는 구간당 16,384자로 제한한다. 전체 청취 시간을 30초로 제한하지 않는다.
- Finish는 남은 PCM을 보낸다. 최초 세션 입력이 2초 미만이면 초기 처리 대기를 해소하기 위한 무음으로 2초까지 보충하고 empty-audio Commit을 한 번 전송한다. 짧은 명령에서의 실제 효과는 실서비스 검증 대상이다.
- **공개 프로토콜에 별도 EOS ACK가 없어 Finish 완료는 서버 전체 처리 완료의 증명이 아니다.** 기본 15초(`FinishDrainSeconds`) 동안 전체 결과를 수신한 뒤 종료한다. 미확정 Partial이 남으면 오류로 끝낸다. 첫 Final 도착만으로 닫지 않지만 수신 구간을 넘겨 도착하는 결과는 보장하지 못한다. 무음에서 공급자가 Commit 오류를 반환하면 이를 숨기지 않고 Error로 전달한다. [공식 SDK의 Commit/Close 구현](https://github.com/elevenlabs/elevenlabs-python/blob/main/src/elevenlabs/realtime/connection.py)
- 오류는 분류 코드 또는 고정 로컬 메시지로 전달한다. 원격 원문·인증 header를 로그에 반사하지 않는다. 자동 재접속/오디오 재전송은 하지 않는다.

## 로컬 검증

TestServer의 잠긴 환경에서 실행한다. 실제 키, 마이크 또는 외부 API를 사용하지 않는다.

```powershell
TestServer/.venv/Scripts/python.exe TestServer/run_scribe_tests.py --engine "<UE-root>" --project "<host.uproject>"
```

`JWNetworkUtility.Scribe.Protocol`은 누적문 교정·구간 ID·중복 metadata·Final-only·취소 재진입·프로토콜 오류를 검사한다. `Integration`은 실제 loopback WebSocket으로 PCM16 인코딩·청크·tail·짧은 초기 세션 padding·종료 Commit 1회·지연 Final·한글을 검사한다. runner는 호스트 Speech 및 JWCU 보안 회귀 검사도 함께 선택한다. 실제 Scribe API의 응답 순서·짧은 음성 인식 정확도·무음 Finish 동작은 별도의 실서비스 검증이 필요하다.
