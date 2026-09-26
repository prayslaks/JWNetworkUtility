// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_BFL_ApiClientService.h"
#include "JsonObjectConverter.h"
#include "JWNU_GIS_ApiClientService.h"
#include "JWNU_ApiRequest.h"
#include "JWNU_HttpRequest.h"
#include "UObject/StrongObjectPtr.h"

UJWNU_HttpRequest* UJWNU_BFL_ApiClientService::SendHttpRequest(const UObject* WorldContextObject, EJWNU_HttpMethod InMethod,
	const FString& InURL, const FString& ApiKey, const FString& InContentBody, const TMap<FString, FString>& InQueryParams,
	const FOnHttpResponseBPEvent& InOnHttpResponse, const FOnHttpRequestJobRetryBPEvent& InOnHttpRequestJobRetry)
{
	TStrongObjectPtr<UJWNU_HttpRequest> Request(UJWNU_HttpRequest::CreateHttpRequest(WorldContextObject));
	if (!Request.IsValid())
	{
		InOnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None,
			TEXT("{\"success\":false,\"code\":\"START_FAILED\",\"message\":\"Cannot create HTTP request for this world\"}"));
		return nullptr;
	}
	Request->OnCompletedNative.AddLambda([InOnHttpResponse](const FJWNU_HttpResult& Result)
	{ InOnHttpResponse.ExecuteIfBound(JWNU_IntToHttpStatusCode(Result.StatusCode), Result.ResponseBody); });
	Request->OnFailedNative.AddLambda([InOnHttpResponse](const FJWNU_HttpError& Error)
	{
		if (Error.Code == EJWNU_HttpRequestError::Cancelled)
		{
			InOnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None,
				TEXT("{\"success\":false,\"code\":\"CANCELLED\",\"message\":\"HTTP request cancelled\"}"));
		}
		else if (Error.Response.StatusCode == 0 && !Error.Message.IsEmpty())
		{
			InOnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None,
				TEXT("{\"success\":false,\"code\":\"START_FAILED\",\"message\":\"Cannot start HTTP request\"}"));
		}
		else { InOnHttpResponse.ExecuteIfBound(JWNU_IntToHttpStatusCode(Error.Response.StatusCode), Error.Response.ResponseBody); }
	});
	Request->OnRetryNative.AddLambda([InOnHttpRequestJobRetry](int32 AttemptNumber)
	{ InOnHttpRequestJobRetry.ExecuteIfBound(AttemptNumber); });
	Request->Start(InMethod, InURL, InContentBody, InQueryParams, ApiKey);
	return Request.Get();
}

UJWNU_ApiRequest* UJWNU_BFL_ApiClientService::CallApi(const UObject* WorldContextObject, EJWNU_ServiceType InServiceType,
	EJWNU_HttpMethod InMethod, const FString& InEndpoint, const FString& InContentBody,
	const TMap<FString, FString>& InQueryParams, const FOnHttpResponseBPEvent& InOnHttpResponse,
	const FOnHttpRequestJobRetryBPEvent& InOnHttpRequestJobRetry, bool bRequiresAuth)
{
	TStrongObjectPtr<UJWNU_ApiRequest> Request(UJWNU_ApiRequest::CreateApiRequest(WorldContextObject));
	if (!Request.IsValid())
	{
		InOnHttpResponse.ExecuteIfBound(EJWNU_HttpStatusCode::None,
			TEXT("{\"success\":false,\"code\":\"START_FAILED\",\"message\":\"Cannot create API request for this world\"}"));
		return nullptr;
	}
	Request->OnCompletedNative.AddLambda([InOnHttpResponse](const FJWNU_ApiResult& Result)
	{ InOnHttpResponse.ExecuteIfBound(Result.StatusCode, Result.ResponseBody); });
	Request->OnFailedNative.AddLambda([InOnHttpResponse](const FJWNU_ApiError& Error)
	{
		const FString Body = Error.Code == EJWNU_ApiRequestError::Cancelled
			? TEXT("{\"success\":false,\"code\":\"CANCELLED\",\"message\":\"API request cancelled\"}") : Error.Response.ResponseBody;
		InOnHttpResponse.ExecuteIfBound(Error.Response.StatusCode, Body);
	});
	Request->OnRetryNative.AddLambda([InOnHttpRequestJobRetry](int32 AttemptNumber)
	{ InOnHttpRequestJobRetry.ExecuteIfBound(AttemptNumber); });
	Request->Start(InServiceType, InMethod, InEndpoint, InContentBody, InQueryParams, bRequiresAuth);
	return Request.Get();
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
