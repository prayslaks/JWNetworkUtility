// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_ApiClientService.h"
#include "JWNU_GIS_ApiHostProvider.h"

DEFINE_LOG_CATEGORY(LogJWNU_GIS_ApiClientService);

UJWNU_GIS_ApiClientService* UJWNU_GIS_ApiClientService::Get(const UObject* WorldContextObject)
{
	// 월드 컨텍스트 오브젝트 이상
	if (WorldContextObject == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("월드 컨텍스트 오브젝트 이상!"));
		return nullptr;
	}

	// 월드 획득
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("월드 획득 실패!"));
		return nullptr;
	}

	// 게임인스턴스 획득
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("게임 인스턴스 획득 실패!"));
		return nullptr;
	}
    
	// APIClientService 게임인스턴스 서브시스템 반환
	return GameInstance->GetSubsystem<UJWNU_GIS_ApiClientService>();
}

FString UJWNU_GIS_ApiClientService::BuildRefreshTokenURL() const
{
	FString Host;
	if (UJWNU_GIS_ApiHostProvider::Get(GetWorld())->GetHost(EJWNU_ServiceType::AuthServer, Host))
	{
		return FString::Printf(TEXT("%s/auth/refresh"), *Host);
	}

	PRINT_LOG(LogJWNU_GIS_ApiClientService, Warning, TEXT("ApiHostProvider 획득 실패, 기본 URL 반환"));
	return TEXT("http://localhost:5000/auth/refresh");
}
