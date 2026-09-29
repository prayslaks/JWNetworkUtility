<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# TypeSafe 직접 호출 — System One으로 통합

> 부분 갱신 일자: 2026-09-30 — 전용 모듈·BP 노드·타입·호환 변환 계층 제거.

[Deprecated 2026-09-30] `JWNetworkUtilityTypeSafe` 공개 API는 제거했다. 새 구현과 사용법의 정본은 [SystemOne.md](SystemOne.md)다. 옛 노드·구조체 redirect와 별도 테스트 실행기는 제공하지 않는다.

TypeSafe 서비스 자체는 공통 System One 요청으로 계속 호출할 수 있다.

1. `Make Type Safe Options`로 공식 직접 endpoint와 `jev-latest` 모델 옵션을 만든다.
2. `Create System One Request → 이벤트 바인딩 → Start` 또는 `Call System One API`에 해당 Options와 TypeSafe 키를 전달한다.
3. 환경변수 방식을 사용하려면 같은 Options를 `Start From Environment` 또는 `Call System One API From Environment`에 연결한다. 이 주소에서는 `TYPESAFE_API_KEY`를 읽는다.

공통 Options의 미연결 기본값은 **OpenRouter**다. TypeSafe 직접 호출에는 반드시 TypeSafe 옵션을 연결한다. 기존 BP 타입은 SystemOne 타입으로 교체하고 C++ Build.cs 의존성도 `JWNetworkUtilitySystemOne`으로 변경한다.

검증은 `TestServer/run_systemone_tests.py` 하나로 수행한다. 기존 전송·BP 시나리오는 `JWNetworkUtility.SystemOne.Transport`로 이전했다.
