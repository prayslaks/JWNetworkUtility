<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->
# 유한 바이너리 HTTP 전송

> 부분 갱신 일자: 2026-11-04 — 기존 64KiB 요청 기본값을 보존하며 호출자별 UTF-8 요청 한도(최대 64MiB), 응답 최대 256MiB 지원.

`FJWNU_BinaryHttp::Post`는 World 없는 에디터 호출자가 작업 스레드에서 JSON을 보내고 원본 응답 바이트를 받는 동기 전송이다. 공급자 프로토콜·인증 선택·이미지/오디오 해석은 JWAIUtility가 담당한다. 본문은 UTF-8 바이트 수로 제한한다. 기존 호출자는 마지막 선택 인자 `MaxRequestBytes=65536`을 유지하고 큰 입력을 처리하는 호출자만 상한을 명시한다.

HTTPS 또는 명시적 `127.0.0.1`만 허용하고 redirect·자동 재시도를 하지 않는다. 전송 제한은 최대 300초다. 취소 플래그는 전송 전·수신 반복에서 확인하며 WinHTTP 수신 대기를 즉시 중단하지는 못한다. 호출자는 스레드 완료 후 리소스를 해제한다. 결과는 StatusCode·Error·Bytes와 요청한 응답 헤더만 제공한다.

`JWNetworkUtility.BinaryHttp.Limits`는 기본 한도·UTF-8 바이트 수·최대 요청 크기를 무전송으로 검사한다. `TestServer/run_binary_tests.py`는 loopback에서 70,000바이트 요청 보존, 이진 응답·상한·취소·timeout·redirect/재시도 금지와 기존 효과음 오프라인 검사를 실행한다.
