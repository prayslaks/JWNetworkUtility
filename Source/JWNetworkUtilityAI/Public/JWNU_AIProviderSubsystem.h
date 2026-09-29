// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JWNU_AIProviderTypes.h"
#include "JWNU_AIProviderSubsystem.generated.h"

/** 프로젝트의 저장 키 관리와 GameInstance별 임시 키를 BP에 제공한다. Provider는 모델명이 아닌 OpenAI·OpenRouter 등 키 발급 서비스 ID다. */
UCLASS(meta=(DisplayName="JWNU AI Provider Subsystem"))
class JWNETWORKUTILITYAI_API UJWNU_AIProviderSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void BeginDestroy() override;

	/** 현재 지원하는 고정 공급자 ID 목록을 반환하는 함수. */
	UFUNCTION(BlueprintPure, Category="JWNU|AI Providers")
	TArray<FString> GetSupportedProviders() const;

	/** 비밀 원문을 제외한 공급자의 저장 키 목록을 반환하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	bool ListKeys(const FString& Provider, TArray<FJWNU_APIKeyInfo>& OutKeys);
	/** 키를 추가·수정하는 함수. 빈 Id는 추가, 빈 ApiKey는 수정 시 기존 값 유지이며 사용 키 저장은 이 인스턴스의 임시 키를 해제한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	bool SaveKey(const FString& Provider, FGuid Id, const FString& Label, const FString& ApiKey, FDateTime ExpiresUtc, FGuid& OutId);
	/** 저장 키를 선택하고 이 GameInstance의 임시 덮어쓰기를 해제하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	bool SelectKey(const FString& Provider, FGuid Id);
	/** 로컬 저장 키를 삭제하는 함수. 사용 키 삭제 시 이 인스턴스의 임시 키도 해제한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	bool DeleteKey(const FString& Provider, FGuid Id);
	/** 공급자의 로컬 저장 키 전체와 이 인스턴스의 임시 키를 삭제하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	bool DeleteAllKeys(const FString& Provider);
	/** 현재 GI의 임시 키 또는 선택한 저장 키를 OutApiKey로 반환하는 함수. 읽기 실패 시 false와 빈 문자열이며 원격 인증은 검사하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	bool GetApiKey(const FString& Provider, FString& OutApiKey);
	/** 현재 인스턴스가 비어 있지 않은 사용 키를 읽을 수 있는지 확인하는 함수. 공급자 서버의 키 유효성은 검사하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	bool HasApiKey(const FString& Provider);
	/** 이 GameInstance에서만 사용할 임시 키를 지정하는 함수. 빈 값은 저장 키 조회를 임시 차단한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	bool SetRuntimeApiKey(const FString& Provider, const FString& ApiKey);
	/** 이 인스턴스의 임시 키를 해제하고 저장 키 조회로 복귀하는 함수. */
	UFUNCTION(BlueprintCallable, Category="JWNU|AI Providers")
	void ClearRuntimeApiKey(const FString& Provider);
	/** 이 인스턴스에 빈 값 차단을 포함한 임시 키 설정이 있는지 반환하는 함수. */
	UFUNCTION(BlueprintPure, Category="JWNU|AI Providers")
	bool HasRuntimeOverride(const FString& Provider) const;

private:
	friend class FJWNU_AIProviderSubsystemTest;
	FString StorageNamespace;
	TMap<FString, FString> RuntimeKeys;
	bool PrepareProvider(const FString& Provider);
	void ClearRuntimeKeys();
};
