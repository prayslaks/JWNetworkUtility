// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_BFL_ApiClientService.h"
#include "JsonObjectConverter.h"
#include "JWNU_GIS_ApiClientService.h"

void UJWNU_BFL_ApiClientService::SendHttpRequest(
	const UObject* WorldContextObject, 
	const EJWNU_HttpMethod InMethod, 
	const FString& InURL, 
	const FString& InAuthToken, 
	const FString& InContentBody, 
	const TMap<FString, FString>& InQueryParams, 
	const FOnHttpResponseBP InOnHttpResponseBP)
{
	// 블루프린트 이벤트를 다시 델리게이트로 감싼다
	const FOnHttpResponseDelegate OnHttpResponse = WrapOnHttpResponseBP(InOnHttpResponseBP);
	
	// HTTP 리퀘스트
	UJWNU_GIS_HttpClientHelper::SendReqeust_RawResponse(WorldContextObject, InMethod, InURL, InAuthToken, InContentBody, InQueryParams, OnHttpResponse);
}

void UJWNU_BFL_ApiClientService::GetAccessToken(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, EJWNU_TokenExtractionResult& OutTokenExtractionResult, FString& OutAccessToken)
{
	if (const auto Subsystem = UJWNU_GIS_ApiTokenProvider::Get(WorldContextObject))
	{
		Subsystem->GetAccessToken(InServiceType, OutTokenExtractionResult, OutAccessToken);
	}
}

void UJWNU_BFL_ApiClientService::GetRefreshToken(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, EJWNU_TokenExtractionResult& OutTokenExtractionResult, FString& OutRefreshToken)
{
	if (const auto Subsystem = UJWNU_GIS_ApiTokenProvider::Get(WorldContextObject))
	{
		Subsystem->GetRefreshToken(InServiceType, OutTokenExtractionResult, OutRefreshToken);
	}
}

bool UJWNU_BFL_ApiClientService::GetTokenContainer(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, FJWNU_AccessTokenContainer& OutTokenContainer)
{
	if (const auto Subsystem = UJWNU_GIS_ApiTokenProvider::Get(WorldContextObject))
	{
		return Subsystem->GetTokenContainer(InServiceType, OutTokenContainer);
	}
	return false;	
}

bool UJWNU_BFL_ApiClientService::ConvertJsonStringToStruct(const FString& JsonString, int32& OutStruct)
{
	checkNoEntry();
	return false;
}
bool UJWNU_BFL_ApiClientService::Generic_ConvertJsonStringToStruct(const FString& JsonString, const FProperty* StructProperty, void* StructPtr)
{
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		if (const FStructProperty* StructProp = CastField<FStructProperty>(StructProperty))
		{
			return FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), StructProp->Struct, StructPtr);
		}
	}
	return false;
}

bool UJWNU_BFL_ApiClientService::SaveTokenWithEncryption(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, const FString& InToken)
{
	if (const auto Subsystem = UJWNU_GIS_ApiTokenProvider::Get(WorldContextObject))
	{
		return Subsystem->SaveRefreshTokenWithEncryption(InServiceType, InToken);
	}
	return false;
}

bool UJWNU_BFL_ApiClientService::LoadTokenWithDecryption(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType, FString& OutToken)
{
	if (const auto Subsystem = UJWNU_GIS_ApiTokenProvider::Get(WorldContextObject))
	{
		return Subsystem->LoadRefreshTokenWithDecryption(InServiceType, OutToken);
	}
	return false;
}

FOnHttpResponseDelegate UJWNU_BFL_ApiClientService::WrapOnHttpResponseBP(FOnHttpResponseBP OnHttpResponseBP)
{
	const FOnHttpResponseDelegate Wrapper = FOnHttpResponseDelegate::CreateLambda([OnHttpResponseBP](const FString& RawResponseBody)
	{
		OnHttpResponseBP.ExecuteIfBound(RawResponseBody);
	});
	return Wrapper;
}
