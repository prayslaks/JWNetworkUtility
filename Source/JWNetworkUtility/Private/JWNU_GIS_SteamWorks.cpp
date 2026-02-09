// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_GIS_SteamWorks.h"

#include "JWNetworkUtility.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"

DEFINE_LOG_CATEGORY(JW_GIS_SteamWorks);

void UJWNU_GIS_SteamWorks::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	SCOPE_WALL()
	
	// 1. 온라인 서브시스템 가져오기
	const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
	if (Subsystem == nullptr)
	{
		PRINT_LOG(JW_GIS_SteamWorks, Warning, TEXT("온라인 서브시스템 획득 실패!"));
		return;	
	}
	
	// 2. 온라인 ID 인터페이스 가져오기
	const IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
	if (Identity.IsValid() == false)
	{
		PRINT_LOG(JW_GIS_SteamWorks, Warning, TEXT("온라인 ID 인터페이스 획득 실패!"));
		return;
	}
	
	// 3. 로컬 유저의 UniqueNetId 가져오기 (0번 로컬 유저)
	const TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);
	const FString SteamIdString = UserId->ToString();
	if (UserId.IsValid() == false || SteamIdString.IsEmpty())
	{
		PRINT_LOG(JW_GIS_SteamWorks, Warning, TEXT("로컬 유저의 UniqueNetId 획득 실패!"));
		return;
	}
	
	// 4. Steam ID를 문자열로 변환하여 출력
	PRINT_LOG(JW_GIS_SteamWorks, Display, TEXT("로컬 유저 0번의 스팀 ID : %s"), *SteamIdString)

	// 5. OSS Steam 기준 GetAuthToken은 비동기로 동작하지 않고 즉시 티켓을 반환
	const FString AuthTicket = Identity->GetAuthToken(0);
	if (AuthTicket.IsEmpty())
	{
		PRINT_LOG(JW_GIS_SteamWorks, Warning, TEXT("스팀 인증 티켓 획득 실패!"));
		return;
	}
	
	PRINT_LOG(JW_GIS_SteamWorks, Display, TEXT("스팀 인증 티켓 데이터 : %s"), *AuthTicket);
	
	// 6. 스팀 인증 티켓을 자체 서버로 전송
}