// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "JWNetworkUtilityTypes.h"
#include "JWNetworkUtilityDelegates.generated.h"

/**
 * 하위 레이어 : HTTP 리퀘스트 재시도 발생 시 호출되는 델리게이트. 
 * @param AttemptNumber 다음 시도 번호 (2부터 시작)
 */
DECLARE_DELEGATE_OneParam(FOnHttpRequestJobRetryDelegate, const int32 /**AttemptNumber*/);

/**
 * 상위 레이어 : HTTP 리퀘스트 재시도 발생 시 호출되는 BP 이벤트.
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnHttpRequestJobRetryBPEvent, const int32, AttemptNumber);

/** 
 * 하위 레이어 : HTTP 리퀘스트의 (네트워크 상태, 상태 코드, 리스폰스 바디)를 패러미터로 받는 델리게이트.
 */
DECLARE_DELEGATE_ThreeParams(FOnHttpRequestJobCompletedDelegate, const bool /*bNetworkAvailable*/, const int32 /*StatusCode*/, const FString& /*ResponseBody*/)

/**
 * 중위 레이어 : HTTP 리퀘스트의 (상태 코드, 리스폰스 바디)를 패러미터로 받는 델리게이트.
 */
DECLARE_DELEGATE_TwoParams(FOnHttpRequestCompletedDelegate, const int32 /*StatusCode*/, const FString& /*ResponseBody*/)

/**
 * 상위 레이어 : HTTP 리퀘스트의 (HTTP 상태코드 열거형, 리스폰스 바디)를 패러미터로 받는 델리게이트.
 */
DECLARE_DELEGATE_TwoParams(FOnHttpResponseDelegate, const EJWNU_HttpStatusCode /*StatusCode*/, const FString& /*ResponseBody*/)

/**
 * 상위 레이어 : HTTP 리퀘스트의 (HTTP 상태코드 열거형, 리스폰스 바디)를 패러미터로 받는 BP 이벤트.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnHttpResponseBPEvent, EJWNU_HttpStatusCode, StatusCode, FString, ResponseBody);

/**
 * 더미 구조체.
 */
USTRUCT()
struct FJWNU_DummyStruct
{
	GENERATED_BODY()
};
