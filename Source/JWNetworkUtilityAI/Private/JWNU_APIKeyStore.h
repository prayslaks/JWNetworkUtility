// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"

/** AI 키 저장소 전용 암호문 파일 입출력. 외부 모듈에는 노출하지 않는다. */
class FJWNU_APIKeyStore
{
public:
	static bool Save(const FString& AppId, const FString& KeyName, const FString& ApiKey);
	static bool Load(const FString& AppId, const FString& KeyName, FString& OutApiKey);
	static bool Exists(const FString& AppId, const FString& KeyName);
	static bool Delete(const FString& AppId, const FString& KeyName);
	static bool IsValidIdentifier(const FString& Value);
private:
	friend class FJWNU_APIKeyStoreTest;
	static FString GetStoragePath(const FString& AppId, const FString& KeyName);
	static FString GetContext(const FString& AppId, const FString& KeyName);
};
