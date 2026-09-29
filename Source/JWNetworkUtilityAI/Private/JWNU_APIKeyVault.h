// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_AIProviderTypes.h"

/** 공급자별 여러 키와 사용 키 선택·메타데이터를 하나의 OS 암호문으로 저장한다. 게임 스레드 전용이다. */
class FJWNU_APIKeyVault
{
public:
	/** 저장 키 목록을 원문 없이 조회하는 함수. 손상 시 false이며 빈 목록으로 덮어쓰지 않는다. */
	static bool ListKeys(const FString& AppId, const FString& Group, TArray<FJWNU_APIKeyInfo>& OutKeys);
	/** 키를 추가·편집하는 함수. 새 항목은 빈 Id를 사용하며 편집 시 빈 ApiKey는 기존 원문을 유지한다. */
	static bool SaveKey(const FString& AppId, const FString& Group, FGuid Id, const FString& Label, const FString& ApiKey, FDateTime ExpiresUtc, FGuid& OutId);
	/** 해당 그룹의 사용 키를 선택하는 함수. */
	static bool SelectKey(const FString& AppId, const FString& Group, FGuid Id);
	/** 로컬 키를 삭제하는 함수. 사용 키를 삭제하면 자동 대체하지 않고 선택을 비운다. */
	static bool DeleteKey(const FString& AppId, const FString& Group, FGuid Id);
	/** 그룹의 모든 저장 키를 제거하는 함수. 공급자 서버의 키는 폐기하지 않는다. */
	static bool DeleteAllKeys(const FString& AppId, const FString& Group);
	/** 선택한 저장 키의 원문을 반환하는 함수. */
	static bool GetActiveKey(const FString& AppId, const FString& Group, FString& OutKey);

private:
	struct FEntry { FJWNU_APIKeyInfo Info; FString Secret; };
	static bool Load(const FString& AppId, const FString& Group, TArray<FEntry>& Entries, FGuid& Active);
	static bool Save(const FString& AppId, const FString& Group, const TArray<FEntry>& Entries, const FGuid& Active);
	static void Clear(TArray<FEntry>& Entries);
};
