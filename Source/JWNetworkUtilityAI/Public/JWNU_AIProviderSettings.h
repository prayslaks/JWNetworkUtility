// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "JWNU_AIProviderTypes.h"
#include "JWNU_AIProviderSettings.generated.h"

/** 코드에서만 관리하는 공급자 정의다. 지원 종료 항목은 저장 키 조회·삭제를 위해 목록에 남긴다. */
struct FJWNU_AIProviderDefinition
{
	FString Id;
	bool bDeprecated = false;
};

/** 프로젝트별 AI 공급자 목록과 개인 암호화 키 관리 화면을 제공한다. 비밀값은 Config에 저장하지 않는다. */
UCLASS(meta=(DisplayName="AI Provider Settings"))
class JWNETWORKUTILITYAI_API UJWNU_AIProviderSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("JWNetworkUtility"); }
	virtual FName GetSectionName() const override { return TEXT("AIProviders"); }

	/** 지원 종료 항목을 포함한 고정 공급자 목록을 반환하는 함수. */
	static const TArray<FJWNU_AIProviderDefinition>& GetProviderCatalog();
	/** 저장 키 관리 대상에 등록된 공급자인지 반환하는 함수. */
	static bool IsKnownProvider(const FString& Provider);
	/** 새 키 등록과 런타임 사용을 지원하는 공급자인지 반환하는 함수. */
	static bool IsSupportedProvider(const FString& Provider);

	/** 현재 프로젝트 이름을 저장소 네임스페이스로 반환하는 함수. */
	FString GetStorageNamespace() const;
	/** 저장 키의 비밀값을 제외한 목록을 조회하는 함수. */
	bool GetSavedKeys(const FString& Provider, TArray<FJWNU_APIKeyInfo>& OutKeys) const;
	/** 키를 추가하거나 수정하는 함수. 수정 시 빈 비밀값은 기존 값을 유지한다. */
	bool SaveManagedKey(const FString& Provider, FGuid Id, const FString& Label, const FString& Key, FDateTime Expiry, FGuid& OutId) const;
	/** 다음 조회에서 사용할 저장 키를 선택하는 함수. */
	bool SelectManagedKey(const FString& Provider, FGuid Id) const;
	/** 해당 로컬 저장 키만 삭제하는 함수. */
	bool DeleteManagedKey(const FString& Provider, FGuid Id) const;
	/** 에디터 등 GameInstance가 없는 네이티브 호출자에게 저장 키를 반환하는 함수. */
	bool GetApiKey(const FString& Provider, FString& OutKey) const;
};
