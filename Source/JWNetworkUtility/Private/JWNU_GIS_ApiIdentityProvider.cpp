// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_ApiIdentityProvider.h"
#include "JWNetworkUtility.h"
#include "JsonObjectConverter.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <dpapi.h>

#pragma comment(lib, "crypt32.lib")

DEFINE_LOG_CATEGORY(LogJWNU_GIS_ApiIdentityProvider);

void UJWNU_GIS_ApiIdentityProvider::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ServiceTypeToTokenContainerMap.Emplace(EJWNU_ServiceType::GameServer, {});
	ServiceTypeToTokenContainerMap.Emplace(EJWNU_ServiceType::AuthServer, {});
}

UJWNU_GIS_ApiIdentityProvider* UJWNU_GIS_ApiIdentityProvider::Get(const UObject* WorldContextObject)
{
	// 월드 컨텍스트 오브젝트 이상
	if (WorldContextObject == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("월드 컨텍스트 오브젝트 이상!"));
		return nullptr;
	}

	// 월드 획득
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("월드 획득 실패!"));
		return nullptr;
	}

	// 게임인스턴스 획득
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("게임인스턴스 획득 실패!"));
		return nullptr;
	}

	// IdentityProvider 게임인스턴스 서브시스템 반환
	return GameInstance->GetSubsystem<UJWNU_GIS_ApiIdentityProvider>();
}

bool UJWNU_GIS_ApiIdentityProvider::GetAccessTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenGetResult& OutTokenGetResult, FJWNU_AccessTokenContainer& OutAccessTokenContainer) const
{
	if (ServiceTypeToTokenContainerMap.Find(InServiceType))
	{
		OutAccessTokenContainer = ServiceTypeToTokenContainerMap[InServiceType];
		if (OutAccessTokenContainer.AccessToken.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("Extraction of AccessToken is Successful, but it's Empty! - 엑세스 토큰 추출에 성공했지만 내용물이 비어있습니다!"));
			OutTokenGetResult = EJWNU_TokenGetResult::Empty;
			return true;
		}

		// 단, 내용물이 유효한지는 보장할 수 없다
		OutTokenGetResult = EJWNU_TokenGetResult::Success;
		return true;
	}

	// 기본 생성자의 트래시 토큰이 담겨있다
	OutTokenGetResult = EJWNU_TokenGetResult::Fail;
	return false;
}

bool UJWNU_GIS_ApiIdentityProvider::GetAccessTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_AccessTokenContainer& OutAccessTokenContainer) const
{
	if (ServiceTypeToTokenContainerMap.Find(InServiceType))
	{
		OutAccessTokenContainer = ServiceTypeToTokenContainerMap[InServiceType];
		if (OutAccessTokenContainer.AccessToken.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("Extraction of AccessToken is Successful, but it's Empty! - 엑세스 토큰 추출에 성공했지만 내용물이 비어있습니다!"));
		}

		// 단, 내용물이 유효한지는 보장할 수 없다
		return true;
	}

	// 기본 생성자의 트래시 토큰이 담겨있다
	return false;
}

bool UJWNU_GIS_ApiIdentityProvider::SetAccessTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenSetResult& OutTokenSetResult, const FJWNU_AccessTokenContainer& InAccessTokenContainer)
{
	ServiceTypeToTokenContainerMap.Add(InServiceType, InAccessTokenContainer);
	PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Display, TEXT("Token Container Updated for Service Type - 서비스 타입의 토큰 컨테이너가 갱신되었습니다!"));
	OutTokenSetResult = EJWNU_TokenSetResult::Success;
	return true;
}

bool UJWNU_GIS_ApiIdentityProvider::SetAccessTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_AccessTokenContainer& InTokenContainer)
{
	ServiceTypeToTokenContainerMap.Add(InServiceType, InTokenContainer);
	PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Display, TEXT("Token Container Updated for Service Type - 서비스 타입의 토큰 컨테이너가 갱신되었습니다!"));
	return true;
}

bool UJWNU_GIS_ApiIdentityProvider::GetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenGetResult& OutTokenGetResult, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer) const
{
	if (LoadRefreshTokenContainer(InServiceType, OutRefreshTokenContainer))
	{
		if (OutRefreshTokenContainer.RefreshToken.IsEmpty())
		{
			OutTokenGetResult = EJWNU_TokenGetResult::Empty;
			PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("리프레시 토큰 컨테이너를 획득했지만 토큰이 비어있습니다!"));
		}

		// 단, 내용물이 유효한지는 보장할 수 없다
		OutTokenGetResult = EJWNU_TokenGetResult::Success;
		return true;
	}

	// 기본 생성자의 트래시 토큰이 담겨있다
	OutTokenGetResult = EJWNU_TokenGetResult::Fail;
	return false;
}

bool UJWNU_GIS_ApiIdentityProvider::GetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer)
{
	if (LoadRefreshTokenContainer(InServiceType, OutRefreshTokenContainer))
	{
		if (OutRefreshTokenContainer.RefreshToken.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("리프레시 토큰 컨테이너를 획득했지만 토큰이 비어있습니다!"));
		}
		return true;
	}
	return false;
}

bool UJWNU_GIS_ApiIdentityProvider::SetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, EJWNU_TokenSetResult& OutTokenSetResult, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer)
{
	if (SaveRefreshTokenContainer(InServiceType, InRefreshTokenContainer))
	{
		OutTokenSetResult = EJWNU_TokenSetResult::Success;
		return true;
	}

	OutTokenSetResult = EJWNU_TokenSetResult::Fail;
	return false;
}

bool UJWNU_GIS_ApiIdentityProvider::SetRefreshTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer)
{
	return SaveRefreshTokenContainer(InServiceType, InRefreshTokenContainer);
}

// ──────── UserId ────────

FString UJWNU_GIS_ApiIdentityProvider::GetUserId() const
{
	return UserId;
}

void UJWNU_GIS_ApiIdentityProvider::SetUserId(const FString& InUserId)
{
	UserId = InUserId;
	PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Display, TEXT("UserId 설정됨: %s"), *UserId);
}

// ──────── 세션 정리 ────────

void UJWNU_GIS_ApiIdentityProvider::ClearSession(const EJWNU_ServiceType InServiceType)
{
	// AccessToken 정리
	ServiceTypeToTokenContainerMap.Add(InServiceType, {});

	PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Display, TEXT("세션 정리 완료 (ServiceType: %s) — UserId 유지"), *UEnum::GetValueAsString(InServiceType));
}

// ──────── Token Encryption ────────

bool UJWNU_GIS_ApiIdentityProvider::SaveRefreshTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_RefreshTokenContainer& InRefreshTokenContainer)
{
	// 컨테이너를 JSON 문자열로 직렬화
	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(InRefreshTokenContainer, JsonString) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("리프레시 토큰 컨테이너 JSON 직렬화 실패!"));
		return false;
	}

	// JSON 문자열 암호화
	TArray<uint8> OutEncryptedData;
	if (EncryptToken(JsonString, OutEncryptedData) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("토큰 암호화 실패!"));
		return false;
	}

	// 서비스에 따른 파일 경로
	const FString ServiceTypeString = UEnum::GetDisplayValueAsText(InServiceType).ToString();
	const FString DirectoryPath = FPaths::ProjectSavedDir() + TEXT("Config/JWNetworkUtility/");
	const FString Path = DirectoryPath + FString::Printf(TEXT("auth_%s.bin"), *ServiceTypeString);
	
	// 폴더가 존재하는지 확인하고 없으면 생성
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.DirectoryExists(*DirectoryPath) == false)
	{
		// CreateDirectoryTree는 하위 폴더까지 한 번에 생성한다.
		if (PlatformFile.CreateDirectoryTree(*DirectoryPath))
		{
			PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Display, TEXT("디렉토리 생성 성공: %s"), *DirectoryPath);
		}
		else
		{
			PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Error, TEXT("디렉토리 생성 실패!"));
		}
	}

	// 파일 저장
	if (FFileHelper::SaveArrayToFile(OutEncryptedData, *Path) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("암호화된 파일 저장 실패!"));
		return false;
	}

	PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Display, TEXT("리프레시 토큰 컨테이너 암호화 및 파일 저장 성공!"));
	return true;
}

bool UJWNU_GIS_ApiIdentityProvider::LoadRefreshTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_RefreshTokenContainer& OutRefreshTokenContainer)
{
	// 서비스에 따른 파일 경로
	const FString ServiceTypeString = UEnum::GetDisplayValueAsText(InServiceType).ToString();
	const FString DirectoryPath = FPaths::ProjectSavedDir() + TEXT("Config/JWNetworkUtility/");
	const FString FullPath = DirectoryPath + FString::Printf(TEXT("auth_%s.bin"), *ServiceTypeString);
	
	// 파일 로드
	TArray<uint8> Result;
	if (FFileHelper::LoadFileToArray(Result, *FullPath) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("암호화된 파일 로드 실패!"));
		return false;
	}

	// 복호화
	FString JsonString;
	if (DecryptToken(Result, JsonString) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("토큰 복호화 실패!"));
		return false;
	}

	// JSON 문자열에서 컨테이너로 역직렬화
	if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &OutRefreshTokenContainer, 0, 0) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Warning, TEXT("리프레시 토큰 컨테이너 JSON 역직렬화 실패!"));
		return false;
	}

	PRINT_LOG(LogJWNU_GIS_ApiIdentityProvider, Display, TEXT("파일 로드 및 리프레시 토큰 컨테이너 복호화 성공!"));
	return true;
}

bool UJWNU_GIS_ApiIdentityProvider::EncryptToken(const FString& InToken, TArray<uint8>& OutEncryptedData)
{
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB DataEntropy;

	// FString을 바이트 배열로 변환
	const FTCHARToUTF8 Converter(*InToken);
	DataIn.pbData = (BYTE*)Converter.Get();
	DataIn.cbData = Converter.Length();

	// 기기 ID를 솔트로 사용하기 위해 변환
	const FString DeviceID = FPlatformMisc::GetDeviceId() + TEXT("JWNetworkUtility");
	const FTCHARToUTF8 EntropyConverter(*DeviceID);
	DataEntropy.pbData = (BYTE*)EntropyConverter.Get();
	DataEntropy.cbData = EntropyConverter.Length();

	// 윈도우 DPAPI 호출 (현재 로그인된 사용자 계정으로 암호화)
	if (CryptProtectData(&DataIn, nullptr, &DataEntropy, nullptr, nullptr, 0, &DataOut))
	{
		OutEncryptedData.Empty();
		OutEncryptedData.Append((uint8*)DataOut.pbData, DataOut.cbData);

		// 메모리 해제 필수
		LocalFree(DataOut.pbData);
		return true;
	}
	return false;
}

bool UJWNU_GIS_ApiIdentityProvider::DecryptToken(const TArray<uint8>& InEncryptedData, FString& OutToken)
{
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB DataEntropy;

	DataIn.pbData = (BYTE*)InEncryptedData.GetData();
	DataIn.cbData = InEncryptedData.Num();

	// 기기 ID를 솔트로 사용하기 위해 변환
	const FString DeviceID = FPlatformMisc::GetDeviceId() + TEXT("JWNetworkUtility");
	const FTCHARToUTF8 EntropyConverter(*DeviceID);
	DataEntropy.pbData = (BYTE*)EntropyConverter.Get();
	DataEntropy.cbData = EntropyConverter.Length();

	// 윈도우 DPAPI 호출 (현재 로그인된 사용자 계정으로 복호화)
	if (CryptUnprotectData(&DataIn, nullptr, &DataEntropy, nullptr, nullptr, 0, &DataOut))
	{
		TArray<uint8> TempData;
		TempData.Append((uint8*)DataOut.pbData, DataOut.cbData);
		TempData.Add(0);

		// 복호화된 데이터를 다시 FString으로 변환
		OutToken = FString(UTF8_TO_TCHAR((char*)TempData.GetData()));

		// 메모리 해제 필수
		LocalFree(DataOut.pbData);
		return true;
	}
	return false;
}
