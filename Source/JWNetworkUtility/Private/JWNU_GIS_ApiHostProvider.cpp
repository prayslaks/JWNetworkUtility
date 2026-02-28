// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_ApiHostProvider.h"
#include "Interfaces/IPluginManager.h"
#include "Engine/GameInstance.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"					// 설정 파일
#include "JWNetworkUtility.h"

DEFINE_LOG_CATEGORY(LogJWNU_GIS_ApiHostProvider);

void UJWNU_GIS_ApiHostProvider::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const FString Section = TEXT("/Script/JWNetworkUtility.JWNU_GIS_ApiHostProvider");
	const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("JWNetworkUtility"));
	if (!Plugin.IsValid())
	{
		PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Error, TEXT("JWNetworkUtility plugin not found!"));
		return;
	}
	const FString PluginConfigPath  = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Config"), TEXT("DefaultJWNetworkUtility.ini"));
	const FString ProjectConfigPath = FPaths::ProjectConfigDir() / TEXT("DefaultJWNetworkUtility.ini");

	const UEnum* ServiceTypeEnum = StaticEnum<EJWNU_ServiceType>();
	if (!ServiceTypeEnum) return;

	for (int32 i = 0; i < ServiceTypeEnum->NumEnums() - 1; i++)
	{
		const EJWNU_ServiceType Type = static_cast<EJWNU_ServiceType>(ServiceTypeEnum->GetValueByIndex(i));

		FString Key = ServiceTypeEnum->GetNameStringByIndex(i);
		const int32 ColonIdx = Key.Find(TEXT("::"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (ColonIdx != INDEX_NONE) Key = Key.Mid(ColonIdx + 2);

		FString Value;
		const bool bFoundInProject = GConfig->GetString(*Section, *Key, Value, ProjectConfigPath);
		if (!bFoundInProject)
		{
			GConfig->GetString(*Section, *Key, Value, PluginConfigPath);
		}

		if (!Value.IsEmpty())
		{
			ServiceTypeToHostMap.Add(Type, Value);
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Display, TEXT("Host address loaded — %s : %s"), *Key, *Value);
		}
		else
		{
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("Failed to load host address — %s : ???"), *Key);
		}
	}
}

UJWNU_GIS_ApiHostProvider* UJWNU_GIS_ApiHostProvider::Get(const UObject* WorldContextObject)
{
	// 월드 컨텍스트 오브젝트 이상
	if (WorldContextObject == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("WorldContextObject is invalid!"));
		return nullptr;
	}

	// 월드 획득
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("Failed to get World!"));
		return nullptr;
	}

	// 게임인스턴스 획득
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("Failed to get GameInstance!"));
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
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("Extraction of Host is Successful, but it's Empty!"));
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
			PRINT_LOG(LogJWNU_GIS_ApiHostProvider, Warning, TEXT("Extraction of Host is Successful, but it's Empty!"));
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
