// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.


#include "JWNU_GIS_CustomCodeHelper.h"

#define LOCTEXT_NAMESPACE "JW_API_CUSTOM_CODE"

void UJWNU_GIS_CustomCodeHelper::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// 매핑
	MapCustomCodeToText();
}

FText UJWNU_GIS_CustomCodeHelper::GetTranslatedTextFromResponseCode(const FString& CustomCode) const
{
	if (const FText* FoundText = CustomCodeToTextMap.Find(CustomCode))
	{
		return *FoundText;
	}
	return FText::Format(LOCTEXT("UNKNOWN_ERROR", "알 수 없는 오류가 발생했습니다. (코드: {0})"), FText::FromString(CustomCode));
}

void UJWNU_GIS_CustomCodeHelper::MapCustomCodeToText()
{
	// 맵을 완전히 비운다
	CustomCodeToTextMap.Empty();

	// HTTP 상태 코드 변환
	CustomCodeToTextMap.Emplace(TEXT("BAD_REQUEST"), LOCTEXT("BAD_REQUEST", "잘못된 요청입니다."));
	CustomCodeToTextMap.Emplace(TEXT("UNAUTHORIZED"), LOCTEXT("UNAUTHORIZED", "인증이 필요합니다."));
	CustomCodeToTextMap.Emplace(TEXT("PAYMENT_REQUIRED"), LOCTEXT("PAYMENT_REQUIRED", "결제가 필요합니다."));
	CustomCodeToTextMap.Emplace(TEXT("FORBIDDEN"), LOCTEXT("FORBIDDEN", "접근이 금지되었습니다."));
	CustomCodeToTextMap.Emplace(TEXT("NOT_FOUND"), LOCTEXT("NOT_FOUND", "찾을 수 없습니다."));
	CustomCodeToTextMap.Emplace(TEXT("METHOD_NOT_ALLOWED"), LOCTEXT("METHOD_NOT_ALLOWED", "허용되지 않은 메소드입니다."));
	CustomCodeToTextMap.Emplace(TEXT("NOT_ACCEPTABLE"), LOCTEXT("NOT_ACCEPTABLE", "수용할 수 없는 요청입니다."));
	CustomCodeToTextMap.Emplace(TEXT("PROXY_AUTH_REQUIRED"), LOCTEXT("PROXY_AUTH_REQUIRED", "프록시 인증이 필요합니다."));
	CustomCodeToTextMap.Emplace(TEXT("REQUEST_TIMEOUT"), LOCTEXT("REQUEST_TIMEOUT", "요청 시간이 초과되었습니다."));
	CustomCodeToTextMap.Emplace(TEXT("INTERNAL_SERVER_ERROR"), LOCTEXT("INTERNAL_SERVER_ERROR", "서버 내부 오류입니다."));
	CustomCodeToTextMap.Emplace(TEXT("NOT_IMPLEMENTED"), LOCTEXT("NOT_IMPLEMENTED", "구현되지 않은 기능입니다."));
	CustomCodeToTextMap.Emplace(TEXT("BAD_GATEWAY"), LOCTEXT("BAD_GATEWAY", "게이트웨이 오류입니다."));
	CustomCodeToTextMap.Emplace(TEXT("SERVICE_UNAVAILABLE"), LOCTEXT("SERVICE_UNAVAILABLE", "서비스를 사용할 수 없습니다."));
	CustomCodeToTextMap.Emplace(TEXT("GATEWAY_TIMEOUT"), LOCTEXT("GATEWAY_TIMEOUT", "게이트웨이 시간 초과입니다."));
	
	// TODO: 프로젝트에 필요한 커스텀 코드를 여기에 추가
}

#undef LOCTEXT_NAMESPACE