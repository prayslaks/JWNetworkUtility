// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWNU_GIS_CustomCodeHelper.generated.h"

/**
 * 커스텀 코드를 게임인스턴스 서브시스템 헬퍼 클래스.
 */
UCLASS()
class JWNETWORKUTILITY_API UJWNU_GIS_CustomCodeHelper : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	/**
	 * 주어진 커스텀 코드에 대해 번역된 텍스트를 반환하는 함수.
	 * @param CustomCode MVE API 리스폰스에서 파싱해서 얻는 코드
	 * @return LOCTEXT에 의해 로컬라이징된 FText. 코드와 매핑되는 값이 없다면 기본값
	 */
	FText GetTranslatedTextFromResponseCode(const FString& CustomCode) const;
	
private:
	/**
	 * 커스텀 코드 문자열에 대해 로컬라이징된 문자열을 반환하는 맵.
	 */
	UPROPERTY()
	TMap<FString, FText> CustomCodeToTextMap;

	/**
	 * 커스텀 코드를 로컬라이징된 문자열로 매핑하는 함수.
	 */
	void MapCustomCodeToText();
};
