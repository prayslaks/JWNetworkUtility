# JWUtility Plugin

## 개요

JWUtility 플러그인은 언리얼 엔진 프로젝트를 위한 유틸리티 기능들을 모아놓은 플러그인입니다.

## 주요 기능

### `UJW_GIS_CustomCodeHelper`

`UJW_GIS_CustomCodeHelper`는 서버 API 응답으로 오는 커스텀 코드나 HTTP 상태 코드를 사용자에게 보여줄 현지화된 텍스트로 변환하는 기능을 제공합니다.

#### 원리

1.  **코드 매핑**: `Source/JWUtility/Private/JW_GIS_CustomCodeHelper.cpp` 파일 내의 `MapCustomCodeToText()` 함수에서 코드(문자열)와 `FText`를 매핑합니다.
    -   `LOCTEXT` 매크로를 사용하여 현지화가 가능한 텍스트를 정의합니다.
    -   게임 인스턴스 초기화 시점에 이 함수가 호출되어 매핑 데이터를 준비합니다.

2.  **텍스트 변환**: `GetTranslatedTextFromResponseCode(FString CustomCode)` 함수를 호출하여, API 응답에서 받은 코드 문자열을 매핑된 `FText`로 변환합니다.
    -   만약 매핑된 코드가 없다면, "알 수 없는 오류" 메시지를 기본값으로 반환합니다.

이를 통해 서버에서 정의한 다양한 에러 코드나 표준 HTTP 상태 코드를 클라이언트에서 일관되고 다국어 지원이 가능한 사용자 메시지로 손쉽게 변환할 수 있습니다.

### `UJW_GIS_HttpClientHelper`

`UJW_GIS_HttpClientHelper`는 언리얼 엔진의 `FHttpModule`을 기반으로 HTTP 통신을 간편하게 사용하도록 돕는 헬퍼 클래스입니다.

#### 원리

1.  **요청 단순화**: `GET`, `POST`, `PUT`, `DELETE` 등 주요 HTTP 메서드에 대한 요청 함수를 제공합니다. JWT 인증 토큰, 쿼리 파라미터, JSON 바디 등을 손쉽게 추가할 수 있습니다.

2.  **통합 응답 처리**: 모든 HTTP 요청의 응답은 `OnResponseReceived` 정적 콜백 함수를 통해 처리됩니다. 이 함수의 핵심 원리는 다음과 같습니다.
    -   **네트워크 오류**: 통신 자체에 실패하면 `bSuccess`는 `false`, 응답 바디는 `"NETWORK_ERROR"`가 됩니다.
    -   **성공 (2xx):** HTTP 상태 코드가 200번대이면 서버 로직에 성공적으로 도달한 것으로 간주합니다. `bSuccess`는 `true`가 되며, 서버가 보낸 원본 JSON 응답 바디를 그대로 전달합니다.
    -   **실패 (2xx 이외):** 4xx, 5xx 등 200번대 외의 상태 코드는 서버 로직에 도달하지 못한 인프라 레벨의 오류로 간주합니다. 이 경우, `bSuccess`는 `false`가 되며 **미리 약속된 형태의 가짜(Fake) JSON 응답을 직접 생성하여** 전달합니다. 이 응답 형식은 본 서버(서버 로직)의 커스텀 리스폰스 스타일입니다.
        ```json
        {"serviceSuccess": false, "customCode": "ERROR_CODE", "customMessage": "Error Message"}
        ```
    
    이러한 처리 방식 덕분에, 실제 서버 로직이 반환한 응답이든, 게이트웨이나 웹 서버가 반환한 오류이든, 클라이언트 로직은 항상 일관된 형태의 응답을 받아 처리할 수 있습니다.

3.  **서버 응답 스타일 요구사항**: 이 헬퍼 클래스를 효과적으로 사용하려면, 연동되는 서버 API는 성공 응답(`2xx` 상태 코드) 시에도 `{"serviceSuccess": true, ...}` 형태의 JSON 응답을 반환하는 등, 클라이언트가 예상하는 응답 스타일을 준수해야 합니다. 비정상적인 응답은 `OnResponseReceived` 함수 내에서 의도치 않은 파싱 오류를 발생시킬 수 있습니다.
