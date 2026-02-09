// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JWNU_ApiTest.generated.h"

/**
 * 클래스 전용 로그 카테고리 선언
 */
DECLARE_LOG_CATEGORY_EXTERN(LogJWNU_ApiTest, Log, All);

UCLASS()
class JWNETWORKUTILITY_API AJWNU_ApiTest : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AJWNU_ApiTest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UFUNCTION()
	void TestHttpClientHelper() const;
	
	UFUNCTION()
	void TestApiClientService() const;
};
