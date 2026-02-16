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
	const FOnHttpResponseBPEvent& InOnHttpResponse,
	const FOnHttpRequestJobRetryBPEvent& InOnHttpRequestJobRetry)
{
	// 블루프린트 이벤트를 다시 델리게이트로 감싼다
	const FOnHttpRequestCompletedDelegate ResponseCallback = FOnHttpRequestCompletedDelegate::CreateLambda([InOnHttpResponse](const int32 StatusCode, const FString& ResponseBody)
	{
		InOnHttpResponse.ExecuteIfBound(JWNU_IntToHttpStatusCode(StatusCode), ResponseBody);
	});
	
	// 블루프린트 이벤트를 다시 델리게이트로 감싼다
	const FOnHttpRequestJobRetryDelegate RetryCallback = FOnHttpRequestJobRetryDelegate::CreateLambda([InOnHttpRequestJobRetry](const int32 AttemptNumber)
	{
		InOnHttpRequestJobRetry.ExecuteIfBound(AttemptNumber);
	});
	
	// HTTP 리퀘스트
	UJWNU_GIS_HttpClientHelper::SendReqeust_RawResponse(WorldContextObject, InMethod, InURL, InAuthToken, InContentBody, InQueryParams, ResponseCallback, RetryCallback);
}

void UJWNU_BFL_ApiClientService::CallApi(
	const UObject* WorldContextObject, 
	const EJWNU_ServiceType InServiceType,
	const EJWNU_HttpMethod InMethod, 
	const FString& InEndpoint, 
	const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams,
	const FOnHttpResponseBPEvent& InOnHttpResponse,
	const FOnHttpRequestJobRetryBPEvent& InOnHttpRequestJobRetry,
	const bool bRequiresAuth)
{
	// 블루프린트 이벤트를 다시 델리게이트로 감싼다
	const FOnHttpResponseDelegate ResponseCallback = FOnHttpResponseDelegate::CreateLambda([InOnHttpResponse](const EJWNU_HttpStatusCode StatusCode, const FString& ResponseBody)
	{
		InOnHttpResponse.ExecuteIfBound(StatusCode, ResponseBody);
	});
	
	// 블루프린트 이벤트를 다시 델리게이트로 감싼다
	const FOnHttpRequestJobRetryDelegate RetryCallback = FOnHttpRequestJobRetryDelegate::CreateLambda([InOnHttpRequestJobRetry](const int32 AttemptNumber)
	{
		InOnHttpRequestJobRetry.ExecuteIfBound(AttemptNumber);
	});
	
	// HTTP 리퀘스트
	UJWNU_GIS_ApiClientService::CallApi_NoTemplate(WorldContextObject, InMethod, InServiceType, InEndpoint, InContentBody, InQueryParams, ResponseCallback, RetryCallback, bRequiresAuth);
}

void UJWNU_BFL_ApiClientService::LoadRefreshTokenContainer(
	const UObject* WorldContextObject,
	const EJWNU_ServiceType InServiceType,
	EJWNU_TokenGetResult& OutTokenGetResult,
	FJWNU_RefreshTokenContainer& OutRefreshTokenContainer)
{
	if (const auto Subsystem = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		Subsystem->GetRefreshTokenContainer(InServiceType, OutTokenGetResult, OutRefreshTokenContainer);
	}
}

void UJWNU_BFL_ApiClientService::SaveRefreshTokenContainer(
	const UObject* WorldContextObject,
	const EJWNU_ServiceType InServiceType,
	EJWNU_TokenSetResult& OutTokenSetResult,
	const FJWNU_RefreshTokenContainer& InRefreshTokenContainer)
{
	if (const auto Subsystem = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		Subsystem->SetRefreshTokenContainer(InServiceType, OutTokenSetResult, InRefreshTokenContainer);
	}
}

void UJWNU_BFL_ApiClientService::GetAccessTokenContainer(
	const UObject* WorldContextObject, 
	const EJWNU_ServiceType InServiceType, 
	EJWNU_TokenGetResult& OutTokenGetResult, 
	FJWNU_AccessTokenContainer& OutAccessTokenContainer)
{
	if (const auto Subsystem = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		Subsystem->GetAccessTokenContainer(InServiceType, OutTokenGetResult, OutAccessTokenContainer);
	}
}

void UJWNU_BFL_ApiClientService::SetAccessTokenContainer(
	const UObject* WorldContextObject,
	const EJWNU_ServiceType InServiceType, 
	EJWNU_TokenSetResult& OutTokenSetResult,
	const FJWNU_AccessTokenContainer& InAccessTokenContainer)
{
	if (const auto Subsystem = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		Subsystem->SetAccessTokenContainer(InServiceType, OutTokenSetResult, InAccessTokenContainer);
	}
}

bool UJWNU_BFL_ApiClientService::GetHost(
	const UObject* WorldContextObject, 
	const EJWNU_ServiceType InServiceType, 
	FString& OutHost)
{
	if (const auto Subsystem = UJWNU_GIS_ApiHostProvider::Get(WorldContextObject))
	{
		return Subsystem->GetHost(InServiceType, OutHost);
	}
	return false;
}

FString UJWNU_BFL_ApiClientService::GetUserId(const UObject* WorldContextObject)
{
	if (const auto Subsystem = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		return Subsystem->GetUserId();
	}
	return TEXT("");
}

void UJWNU_BFL_ApiClientService::SetUserId(const UObject* WorldContextObject, const FString& InUserId)
{
	if (const auto Subsystem = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		Subsystem->SetUserId(InUserId);
	}
}

void UJWNU_BFL_ApiClientService::ClearSession(const UObject* WorldContextObject, const EJWNU_ServiceType InServiceType)
{
	if (const auto Subsystem = UJWNU_GIS_ApiIdentityProvider::Get(WorldContextObject))
	{
		Subsystem->ClearSession(InServiceType);
	}
}

bool UJWNU_BFL_ApiClientService::ConvertJsonStringToStruct(const FString& JsonString, EJWNU_ConvertJsonToStructResult& OutConvertResult, int32& OutStruct)
{
	checkNoEntry();
	return false;
}
bool UJWNU_BFL_ApiClientService::ConvertStructToJsonString(const int32& InStruct, EJWNU_ConvertStructToJsonResult& OutConvertResult, FString& OutJsonString)
{
	checkNoEntry();
	return false;
}
bool UJWNU_BFL_ApiClientService::Generic_ConvertStructToJsonString(const FProperty* StructProperty, const void* StructPtr, EJWNU_ConvertStructToJsonResult& OutConvertResult, FString& OutJsonString)
{
	if (const FStructProperty* StructProp = CastField<FStructProperty>(StructProperty))
	{
		if (FJsonObjectConverter::UStructToJsonObjectString(StructProp->Struct, StructPtr, OutJsonString))
		{
			OutConvertResult = EJWNU_ConvertStructToJsonResult::Success;
			return true;
		}

		OutConvertResult = EJWNU_ConvertStructToJsonResult::Fail;
		return false;
	}

	OutConvertResult = EJWNU_ConvertStructToJsonResult::NoMatch;
	return false;
}
bool UJWNU_BFL_ApiClientService::Generic_ConvertJsonStringToStruct(const FString& JsonString, EJWNU_ConvertJsonToStructResult& OutConvertResult, const FProperty* StructProperty, void* StructPtr)
{
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		if (const FStructProperty* StructProp = CastField<FStructProperty>(StructProperty))
		{
			if (FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), StructProp->Struct, StructPtr))
			{
				OutConvertResult = EJWNU_ConvertJsonToStructResult::Success;
				return true;
			}
			
			OutConvertResult = EJWNU_ConvertJsonToStructResult::Fail;
			return false;
		}
		
		OutConvertResult = EJWNU_ConvertJsonToStructResult::NoMatch;
		return false;
	}
	
	OutConvertResult = EJWNU_ConvertJsonToStructResult::InvalidJSON;
	return false;
}
