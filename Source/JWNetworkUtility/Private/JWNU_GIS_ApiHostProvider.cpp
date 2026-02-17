// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_ApiHostProvider.h"

#include "Interfaces/IPluginManager.h"

#include "JWNetworkUtility.h"

DEFINE_LOG_CATEGORY(LogJWNU_GIS_ApiHostProvider);

void UJWNU_GIS_ApiHostProvider::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const FString Section = TEXT("/Script/JWNetworkUtility.JWNU_GIS_ApiHostProvider");
	const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("JWNetworkUtility"));
	if (!Plugin.IsValid())
	{
		PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Error, TEXT("JWNetworkUtility 플러그인을 찾을 수 없습니다!"));
		return;
	}
	FString PluginDir = Plugin->GetBaseDir();
	const FString ConfigPath = FPaths::Combine(PluginDir, TEXT("Config"), TEXT("DefaultJWNetworkUtility.ini"));
	PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Display, TEXT("호스트 주소 로드 시도 : %s"), *ConfigPath);
	
	auto TryLoad = [&](const EJWNU_ServiceType Type, const TCHAR* Key)
	{
		if (FString Value; GConfig->GetString(*Section, Key, Value, ConfigPath))
		{
			ServiceTypeToHostMap.Add(Type, Value);
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Display, TEXT("호스트 주소 로드 완료 — %s : %s"), Key, *Value);
		}
		else
		{
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("호스트 주소 로드 실패 - %s : ???"), Key);
		}
	};

	TryLoad(EJWNU_ServiceType::GameServer, TEXT("GameServer"));
	TryLoad(EJWNU_ServiceType::AuthServer, TEXT("AuthServer"));
}

UJWNU_GIS_ApiHostProvider* UJWNU_GIS_ApiHostProvider::Get(const UObject* WorldContextObject)
{
	// 월드 컨텍스트 오브젝트 이상
	if (WorldContextObject == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("월드 컨텍스트 오브젝트 이상!"));
		return nullptr;
	}

	// 월드 획득
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("월드 획득 실패!"));
		return nullptr;
	}

	// 게임인스턴스 획득
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("게임인스턴스 획득 실패!"));
		return nullptr;
	}

	// HttpClientHelper 게임인스턴스 서브시스템 반환
	return GameInstance->GetSubsystem<UJWNU_GIS_ApiHostProvider>();
}

bool UJWNU_GIS_ApiHostProvider::GetHost(const EJWNU_ServiceType InServiceType, FString& OutHost) const
{
	if (ServiceTypeToHostMap.Find(InServiceType))
	{
		OutHost = ServiceTypeToHostMap[InServiceType];
		if (OutHost.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("Extraction of Host is Successful, but it's Empty! - 호스트 추출에 성공했지만 내용물이 비어있습니다!"));
			return true;
		}
		
		// 단, 내용물이 유효한지는 보장할 수 없다
		return true;
	}

	// 쓰레기 호스트 반환
	OutHost = TEXT("TRASH_HOST");
	return false;
}

bool UJWNU_GIS_ApiHostProvider::GetHost(const EJWNU_ServiceType InServiceType, EJWNU_HostGetResult& OutHostGetResult, FString& OutHost) const
{
	if (ServiceTypeToHostMap.Find(InServiceType))
	{
		OutHost = ServiceTypeToHostMap[InServiceType];
		if (OutHost.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("Extraction of Host is Successful, but it's Empty! - 호스트 추출에 성공했지만 내용물이 비어있습니다!"));
			OutHostGetResult = EJWNU_HostGetResult::Empty;
			return true;
		}
		
		// 단, 내용물이 유효한지는 보장할 수 없다
		OutHostGetResult = EJWNU_HostGetResult::Success;
		return true;
	}

	// 쓰레기 호스트 반환
	OutHost = TEXT("TRASH_HOST");
	OutHostGetResult = EJWNU_HostGetResult::Fail;
	return false;
}
