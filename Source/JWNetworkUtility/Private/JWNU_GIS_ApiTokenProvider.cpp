// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_ApiTokenProvider.h"
#include "JWNetworkUtility.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <dpapi.h>

#pragma comment(lib, "crypt32.lib")

DEFINE_LOG_CATEGORY(LogJWNU_GIS_ApiTokenProvider);

UJWNU_GIS_ApiTokenProvider* UJWNU_GIS_ApiTokenProvider::Get(const UObject* WorldContextObject)
{
	// 월드 컨텍스트 오브젝트 이상
	if (WorldContextObject == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("월드 컨텍스트 오브젝트 이상!"));
		return nullptr;
	}

	// 월드 획득
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("월드 획득 실패!"));
		return nullptr;
	}

	// 게임인스턴스 획득
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("게임인스턴스 획득 실패!"));
		return nullptr;
	}
    
	// HttpClientHelper 게임인스턴스 서브시스템 반환
	return GameInstance->GetSubsystem<UJWNU_GIS_ApiTokenProvider>();
}

bool UJWNU_GIS_ApiTokenProvider::GetAccessToken(const EJWNU_ServiceType InServiceType, EJWNU_TokenExtractionResult& OutTokenExtractionResult, FString& OutAccessToken) const
{
	if (FJWNU_AccessTokenContainer Result; GetTokenContainer(InServiceType, Result))
	{
		OutAccessToken = Result.AccessToken;
		if (OutAccessToken.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("Extraction of AccessToken is Successful, but it's Empty! - 엑세스 토큰 추출에 성공했지만 내용물이 비어있습니다!"));
			OutTokenExtractionResult = EJWNU_TokenExtractionResult::Empty;
			return true;
		}
		
		// 단, 내용물이 유효한지는 보장할 수 없다
		OutTokenExtractionResult = EJWNU_TokenExtractionResult::Success;
		return true;
	}
	
	// 쓰레기 토큰 반환
	OutAccessToken = TEXT("TRASH_TOKEN");
	OutTokenExtractionResult = EJWNU_TokenExtractionResult::Fail;
	return false;
}

bool UJWNU_GIS_ApiTokenProvider::GetAccessToken(const EJWNU_ServiceType InServiceType, FString& OutAccessToken) const
{
	if (FJWNU_AccessTokenContainer Result; GetTokenContainer(InServiceType, Result))
	{
		OutAccessToken = Result.AccessToken;
		if (OutAccessToken.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("Extraction of AccessToken is Successful, but it's Empty! - 엑세스 토큰 추출에 성공했지만 내용물이 비어있습니다!"));
		}
		
		// 단, 내용물이 유효한지는 보장할 수 없다
		return true;
	}
	
	// 쓰레기 토큰 반환
	OutAccessToken = TEXT("TRASH_TOKEN");
	return false;
}

bool UJWNU_GIS_ApiTokenProvider::GetRefreshToken(const EJWNU_ServiceType InServiceType, EJWNU_TokenExtractionResult& OutTokenExtractionResult, FString& OutRefreshToken) const
{
	if (LoadRefreshTokenWithDecryption(InServiceType, OutRefreshToken))
	{
		if (OutRefreshToken.IsEmpty())
		{
			OutTokenExtractionResult = EJWNU_TokenExtractionResult::Empty;
			PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("리프레시 토큰을 획득했지만 내용물이 비어있습니다!"));
		}
	
		// 단, 내용물이 유효한지는 보장할 수 없다
		OutTokenExtractionResult = EJWNU_TokenExtractionResult::Success;
		return true;
	}
	
	// 쓰레기 토큰 반환
	OutRefreshToken = TEXT("TRASH_TOKEN");
	OutTokenExtractionResult = EJWNU_TokenExtractionResult::Fail;
	return false;
}

bool UJWNU_GIS_ApiTokenProvider::GetRefreshToken(const EJWNU_ServiceType InServiceType, FString& OutRefreshToken) const
{
	if (LoadRefreshTokenWithDecryption(InServiceType, OutRefreshToken))
	{
		if (OutRefreshToken.IsEmpty())
		{
			PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("리프레시 토큰을 획득했지만 내용물이 비어있습니다!"));
		}
	
		// 단, 내용물이 유효한지는 보장할 수 없다
		return true;
	}
	
	// 쓰레기 토큰 반환
	OutRefreshToken = TEXT("TRASH_TOKEN");
	return false;
}

bool UJWNU_GIS_ApiTokenProvider::GetTokenContainer(const EJWNU_ServiceType InServiceType, FJWNU_AccessTokenContainer& OutTokenContainer) const
{
	if (ServiceTypeToTokenContainerMap.Find(InServiceType))
	{
		OutTokenContainer = ServiceTypeToTokenContainerMap[InServiceType];
		return true;
	}

	PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("There isn't Mapping TokenContainer with Service Type - 서비스 타입과 매핑된 토큰 컨테이너가 없습니다!"));
	return false;
}

void UJWNU_GIS_ApiTokenProvider::UpdateTokenContainer(const EJWNU_ServiceType InServiceType, const FJWNU_AccessTokenContainer& InTokenContainer)
{
	ServiceTypeToTokenContainerMap.Add(InServiceType, InTokenContainer);
	PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Display, TEXT("Token Container Updated for Service Type - 서비스 타입의 토큰 컨테이너가 갱신되었습니다!"));
}

bool UJWNU_GIS_ApiTokenProvider::UpdateAccessToken(const EJWNU_ServiceType InServiceType, const FString& InAccessToken)
{
	if (FJWNU_AccessTokenContainer* FoundContainer = ServiceTypeToTokenContainerMap.Find(InServiceType))
	{
		FoundContainer->AccessToken = InAccessToken;
		PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Display, TEXT("Access Token Updated - 엑세스 토큰이 갱신되었습니다!"));
		return true;
	}

	PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("Cannot Update Access Token - No Container Found - 엑세스 토큰 갱신 실패, 컨테이너가 없습니다!"));
	return false;
}

bool UJWNU_GIS_ApiTokenProvider::SaveRefreshTokenWithEncryption(const EJWNU_ServiceType InServiceType, const FString& InRefreshToken)
{
	// 토큰 암호화
	TArray<uint8> OutEncryptedData; 
	if (EncryptToken(InRefreshToken, OutEncryptedData) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("토큰 암호화 실패!"));
		return false;
	}

	// 서비스에 따른 파일 경로
	const FString ServiceTypeString = UEnum::GetDisplayValueAsText(InServiceType).ToString();
	const FString Path = FString::Printf(TEXT("Saved/Config/auth_%s.bin"), *ServiceTypeString);
	
	// 파일 저장
	if (FFileHelper::SaveArrayToFile(OutEncryptedData, *Path) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("암호화된 파일 저장 실패!"));
		return false;
	}
	
	PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Display, TEXT("토큰 암호화 및 파일 저장 성공!"));
	return true;
}

bool UJWNU_GIS_ApiTokenProvider::LoadRefreshTokenWithDecryption(const EJWNU_ServiceType InServiceType, FString& OutRefreshToken)
{
	// 서비스에 따른 파일 경로
	const FString ServiceTypeString = UEnum::GetDisplayValueAsText(InServiceType).ToString();
	const FString Path = FString::Printf(TEXT("Saved/Config/auth_%s.bin"), *ServiceTypeString);
	
	// 파일 로드
	TArray<uint8> Result;
	if (FFileHelper::LoadFileToArray(Result, *Path) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("암호화된 파일 로드 실패!"));
		OutRefreshToken = TEXT("TRASH_TOKEN");
		return false;
	}
	
	// 토큰 복호화
	if (DecryptToken(Result, OutRefreshToken) == false)
	{
		PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Warning, TEXT("토큰 복호화 실패!"));
		OutRefreshToken = TEXT("TRASH_TOKEN");
		return false;
	}
	
	PRINT_LOG(LogJWNU_GIS_ApiTokenProvider, Display, TEXT("파일 로드 및 토큰 복호화 성공!"));
	return true;
}

bool UJWNU_GIS_ApiTokenProvider::EncryptToken(const FString& InToken, TArray<uint8>& OutEncryptedData)
{
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;

	// FString을 바이트 배열로 변환
	const FTCHARToUTF8 Converter(*InToken);
	DataIn.pbData = (BYTE*)Converter.Get();
	DataIn.cbData = Converter.Length();
	
	// 윈도우 DPAPI 호출 (현재 로그인된 사용자 계정으로 암호화)
	if (CryptProtectData(&DataIn, nullptr, nullptr, nullptr, nullptr, 0, &DataOut))
	{
		OutEncryptedData.Empty();
		OutEncryptedData.Append((uint8*)DataOut.pbData, DataOut.cbData);
        
		// 메모리 해제 필수
		LocalFree(DataOut.pbData);
		return true;
	}
	return false;
}

bool UJWNU_GIS_ApiTokenProvider::DecryptToken(const TArray<uint8>& InEncryptedData, FString& OutToken)
{
	DATA_BLOB DataIn;
	DATA_BLOB DataOut;

	DataIn.pbData = (BYTE*)InEncryptedData.GetData();
	DataIn.cbData = InEncryptedData.Num();

	// 윈도우 DPAPI 호출 (현재 로그인된 사용자 계정으로 복호화)
	if (CryptUnprotectData(&DataIn, nullptr, nullptr, nullptr, nullptr, 0, &DataOut))
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
