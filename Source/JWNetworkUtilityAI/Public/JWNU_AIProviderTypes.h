// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNU_AIProviderTypes.generated.h"

/** 원문을 노출하지 않는 저장 키의 관리 정보다. 만료일은 사용자가 지정한 관리 화면 표시용 날짜다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYAI_API FJWNU_APIKeyInfo
{
	GENERATED_BODY()
	/** 키 선택·편집·삭제에 사용하는 저장 항목 GUID 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FGuid Id;
	/** 사용자가 붙인 키 이름 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FString Label;
	/** 원문 대신 보여 주는 마스킹된 끝자리 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FString MaskedKey;
	/** 공유 저장 목록에서 선택된 키인지 나타내며 GI별 임시 키 설정과 구분하는 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") bool bActive = false;
	/** 이 저장소 등록 시각(UTC) 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FDateTime CreatedUtc;
	/** 마지막 편집 시각(UTC) 필드. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FDateTime UpdatedUtc;
	/** 관리용 만료 시각(UTC) 필드. 최소 날짜는 미지정이며 API 사용을 차단하지 않는다. */
	UPROPERTY(BlueprintReadOnly, Category="API Keys") FDateTime ExpiresUtc;
};
