// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "JWNU_OpenAITranscriptionTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace JWNU::OpenAITranscription
{
inline const TCHAR* DelayName(EJWNU_OpenAITranscriptionDelay Delay)
{
    switch (Delay)
    {
    case EJWNU_OpenAITranscriptionDelay::Minimal: return TEXT("minimal");
    case EJWNU_OpenAITranscriptionDelay::Low: return TEXT("low");
    case EJWNU_OpenAITranscriptionDelay::Medium: return TEXT("medium");
    case EJWNU_OpenAITranscriptionDelay::High: return TEXT("high");
    case EJWNU_OpenAITranscriptionDelay::XHigh: return TEXT("xhigh");
    default: return TEXT("");
    }
}
/** 언어·용어 힌트 공통 형식: 공백뿐이거나 개행·< > 를 포함하면 거절한다. */
inline bool IsHint(const FString& Value)
{
    return !Value.TrimStartAndEnd().IsEmpty() && !Value.Contains(TEXT("<")) && !Value.Contains(TEXT(">"))
        && !Value.Contains(TEXT("\r")) && !Value.Contains(TEXT("\n"));
}
/** 형식만 검사한다. 모델별 필드 지원 여부는 공식 문서가 자주 바뀌므로 서버 판정에 맡긴다. */
inline bool ValidOptions(const FJWNU_OpenAITranscriptionOptions& Options)
{
    if (Options.Model.IsEmpty() || Options.Model.Contains(TEXT(" ")) || !IsHint(Options.Model)) { return false; }
    if (static_cast<uint8>(Options.Delay) > static_cast<uint8>(EJWNU_OpenAITranscriptionDelay::XHigh)
        || !FMath::IsFinite(Options.StartTimeoutSeconds) || Options.StartTimeoutSeconds <= 0
        || !FMath::IsFinite(Options.CloseTimeoutSeconds) || Options.CloseTimeoutSeconds <= 0) { return false; }
    if (!Options.Language.IsEmpty() && !IsHint(Options.Language)) { return false; }
    for (const auto& Language : Options.Languages) { if (!IsHint(Language)) { return false; } }
    for (const auto& Keyword : Options.Keywords) { if (!IsHint(Keyword)) { return false; } }
    return true;
}
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
inline FString String(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key)
{
    FString Result;
    if (Object) { Object->TryGetStringField(Key, Result); }
    return Result;
}
inline TSharedRef<FJsonObject> UpdateEvent(const FJWNU_OpenAITranscriptionOptions& Options)
{
    auto Root = Event(TEXT("session.update"));
    auto Session = MakeShared<FJsonObject>();
    Session->SetStringField(TEXT("type"), TEXT("transcription"));
    auto Format = MakeShared<FJsonObject>();
    Format->SetStringField(TEXT("type"), TEXT("audio/pcm"));
    Format->SetNumberField(TEXT("rate"), 24000);
    auto Transcription = MakeShared<FJsonObject>();
    Transcription->SetStringField(TEXT("model"), Options.Model);
    if (Options.Delay != EJWNU_OpenAITranscriptionDelay::Default) { Transcription->SetStringField(TEXT("delay"), DelayName(Options.Delay)); }
    if (!Options.Language.IsEmpty()) { Transcription->SetStringField(TEXT("language"), Options.Language); }
    if (!Options.Prompt.IsEmpty()) { Transcription->SetStringField(TEXT("prompt"), Options.Prompt); }
    TArray<TSharedPtr<FJsonValue>> Languages, Keywords;
    for (const auto& Value : Options.Languages) { Languages.Add(MakeShared<FJsonValueString>(Value)); }
    for (const auto& Value : Options.Keywords) { Keywords.Add(MakeShared<FJsonValueString>(Value)); }
    if (!Languages.IsEmpty()) { Transcription->SetArrayField(TEXT("languages"), Languages); }
    if (!Keywords.IsEmpty()) { Transcription->SetArrayField(TEXT("keywords"), Keywords); }
    auto Input = MakeShared<FJsonObject>();
    Input->SetObjectField(TEXT("format"), Format);
    Input->SetObjectField(TEXT("transcription"), Transcription);
    Input->SetField(TEXT("turn_detection"), MakeShared<FJsonValueNull>());
    auto Audio = MakeShared<FJsonObject>();
    Audio->SetObjectField(TEXT("input"), Input);
    Session->SetObjectField(TEXT("audio"), Audio);
    Root->SetObjectField(TEXT("session"), Session);
    return Root;
}
inline bool MatchesSession(const TSharedPtr<FJsonObject>& Session, const FJWNU_OpenAITranscriptionOptions& Options)
{
    const TSharedPtr<FJsonObject>* Audio = nullptr;
    const TSharedPtr<FJsonObject>* Input = nullptr;
    const TSharedPtr<FJsonObject>* Format = nullptr;
    const TSharedPtr<FJsonObject>* Transcription = nullptr;
    double Rate = 0;
    return Session && !String(Session, TEXT("id")).IsEmpty() && String(Session, TEXT("type")) == TEXT("transcription")
        && Session->TryGetObjectField(TEXT("audio"), Audio) && (*Audio)->TryGetObjectField(TEXT("input"), Input)
        && (*Input)->TryGetObjectField(TEXT("format"), Format) && String(*Format, TEXT("type")) == TEXT("audio/pcm")
        && (*Format)->TryGetNumberField(TEXT("rate"), Rate) && Rate == 24000
        && (*Input)->TryGetObjectField(TEXT("transcription"), Transcription)
        && String(*Transcription, TEXT("model")) == Options.Model
        && (*Input)->HasTypedField<EJson::Null>(TEXT("turn_detection"));
}
}
