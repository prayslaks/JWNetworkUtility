// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JWNU_Actor_ApiTest.generated.h"

/**
 * 클래스 전용 로그 카테고리 선언
 */
DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_Actor_ApiTest, Log, All);

UCLASS()
class JWNETWORKUTILITYTEST_API AJWNU_Actor_ApiTest : public AActor
{
	GENERATED_BODY()

public:
	AJWNU_Actor_ApiTest();

protected:
	virtual void BeginPlay() override;

	/**
	 * HttpClientHelper 기능 테스트 함수.
	 */
	UFUNCTION()
	void TestOne_HttpClientHelper() const;

	/**
	 * ApiClientService 기능 테스트 함수.
	 */
	UFUNCTION()
	void TestTwo_ApiClientService_Template() const;

	/**
	 * ApiClientService 기능 테스트 함수.
	 */
	UFUNCTION()
	void TestThree_ApiClientService_NoTemplate() const;
};
