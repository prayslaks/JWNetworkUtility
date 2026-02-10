// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "Modules/ModuleManager.h"

class FJWNetworkUtilityModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

// 디버깅 로그 카테고리 선언
DECLARE_LOG_CATEGORY_EXTERN(JWLog, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(JWLogWall, Log, All);

/**
 * 현재 함수와 라인 정보를 문자열로 변환하는 매크로.
 */
#define CALL_INFO (FString(__FUNCTION__) + TEXT("(") + FString::FromInt(__LINE__) + TEXT(")"))

/**
 * UE_LOG에 호출 위치를 포함하도록 포장하는 매크로.
 * @param cat 로그 카테고리
 * @param ver 로그 상세
 * @param fmt 언리얼 TEXT 문자열 포매팅, 가변 인자로 입력 가능
 */
#define PRINT_LOG(cat, ver, fmt, ...) \
{ \
const FString __LogMessage__ = FString::Printf(fmt, ##__VA_ARGS__); \
const FString __FullMessage__ = FString::Printf(TEXT("%s : %s"), *CALL_INFO, *__LogMessage__); \
UE_LOG(cat, ver, TEXT("%s"), *__FullMessage__); \
}

#pragma region ScreenDebugger

/**
 * 플러그인 내부 온스크린 디버그 메시지 활성화 콘솔 변수.
 * 콘솔에서 JWNU.DebugScreen 1/0 으로 제어.
 */
extern JWNETWORKUTILITY_API TAutoConsoleVariable<bool> CVarJWNU_DebugScreen;

/**
 * 콘솔 변수로 제어 가능한 온스크린 디버그 메시지 매크로.
 * @param Key 메시지 키 (-1이면 매번 새 줄)
 * @param Duration 표시 시간 (초)
 * @param Color FColor 색상
 * @param fmt TEXT() 포맷 문자열 + 가변 인자
 */
#define JWNU_SCREEN_DEBUG(Key, Duration, Color, fmt, ...) \
{ \
	if (CVarJWNU_DebugScreen.GetValueOnGameThread() && GEngine) \
	{ \
		const FString __ScreenMsg__ = FString::Printf(fmt, ##__VA_ARGS__); \
		GEngine->AddOnScreenDebugMessage(Key, Duration, Color, __ScreenMsg__); \
	} \
}

#pragma endregion ScreenDebugger

#pragma region NetLogger

// 네트워크 디버깅 로그 카테고리 선언
DECLARE_LOG_CATEGORY_EXTERN(JWNetLog, Log, All);

// 네트워크 정보를 문자열로
#define NET_MODE_INFO \
((GetNetMode() == NM_Client) ? *FString::Printf(TEXT("[CLIENT %d]"), UE::GetPlayInEditorID()) \
: ((GetNetMode() == ENetMode::NM_Standalone) ? TEXT("[STANDALONE]") : TEXT("[SERVER]")))

// 간단한 디버깅용 호출 위치 로그
#define PRINT_NET_INFO() UE_LOG(JWNetLog, Display, TEXT("[%s] %s"), NET_MODE_INFO, *CALL_INFO)

// 로그 + 파일 출력
#define PRINT_NET_LOG(fmt, ...) \
{ \
const FString __NetMessage__ = FString(NET_MODE_INFO); \
const FString __LogMessage__ = FString::Printf(fmt, ##__VA_ARGS__); \
const FString __FullMessage__ = FString::Printf(TEXT("[%s] %s : %s"), *__NetMessage__, *CALL_INFO, *__LogMessage__); \
UE_LOG(JWNetLog, Display, TEXT("%s"), *__FullMessage__); \
}

#pragma endregion NetLogger

#pragma region ScopeWallLogger

// 범위 로그 벽의 총 길이를 설정
#define LOG_WALL_WIDTH 60

struct FScopeWallLogger
{
	FString FunctionName;
	explicit FScopeWallLogger(const char* InFuncName) : FunctionName(InFuncName)
	{
		const int32 TextLen = FunctionName.Len() + 6;
		const int32 SideLen = FMath::Max(0, (LOG_WALL_WIDTH - TextLen) / 2);
		const FString Padding = FString::ChrN(SideLen, '=');
		UE_LOG(JWLogWall, Warning, TEXT("%s < %s > %s"), *Padding, *FunctionName, *Padding);
		UE_LOG(JWLogWall, Warning, TEXT(""));
	}
	~FScopeWallLogger()
	{
		// 함수가 끝날 때(Scope를 벗어날 때) 자동 호출
		UE_LOG(JWLogWall, Warning, TEXT(""));
		const FString Padding = FString::ChrN(LOG_WALL_WIDTH, '=');
		UE_LOG(JWLogWall, Warning, TEXT("%s"), *Padding);
	}
};

// 매크로 정의
#define SCOPE_WALL() FScopeWallLogger WallLogger(__FUNCTION__);

#pragma endregion ScopeWallLogger