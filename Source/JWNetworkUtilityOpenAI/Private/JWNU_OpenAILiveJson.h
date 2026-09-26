// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_OpenAILiveTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace JWNU::OpenAILive
{
inline TSharedRef<FJsonObject> Event(const FString& Type)
{
    auto Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("type"), Type);
    Object->SetStringField(TEXT("event_id"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
    return Object;
}
inline FString Encode(const TSharedRef<FJsonObject>& Object)
{
    FString Result;
    FJsonSerializer::Serialize(Object, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Result));
    return Result;
}
inline bool Decode(const FString& Json, TSharedPtr<FJsonObject>& Object)
{
    return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) && Object.IsValid();
}
inline FString String(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
{
    FString Result;
    if (Object) { Object->TryGetStringField(Key, Result); }
    return Result;
}
inline TSharedRef<FJsonObject> StartEvent(const FJWNU_OpenAILiveOptions& Options)
{
    auto Root = Event(TEXT("session.start"));
    auto Session = MakeShared<FJsonObject>();
    Session->SetStringField(TEXT("model"), Options.Model);
    Session->SetStringField(TEXT("instructions"), Options.Instructions);
    auto Format = MakeShared<FJsonObject>();
    Format->SetStringField(TEXT("type"), TEXT("audio/pcm"));
    Format->SetNumberField(TEXT("rate"), Options.SampleRate);
    auto Output = MakeShared<FJsonObject>();
    Output->SetStringField(TEXT("voice"), Options.Voice);
    auto Audio = MakeShared<FJsonObject>();
    Audio->SetObjectField(TEXT("format"), Format);
    Audio->SetObjectField(TEXT("output"), Output);
    Session->SetObjectField(TEXT("audio"), Audio);
    auto Responses = MakeShared<FJsonObject>();
    Responses->SetStringField(TEXT("model"), Options.BackendModel);
    auto Delegation = MakeShared<FJsonObject>();
    Delegation->SetStringField(TEXT("type"), TEXT("responses"));
    Delegation->SetObjectField(TEXT("responses"), Responses);
    Session->SetObjectField(TEXT("delegation"), Delegation);
    Root->SetObjectField(TEXT("session"), Session);
    return Root;
}
}
