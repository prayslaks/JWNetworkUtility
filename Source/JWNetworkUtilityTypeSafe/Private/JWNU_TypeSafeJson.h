// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "JWNU_TypeSafeTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace JWNU::TypeSafeJson
{
inline bool ReadValue(const FString& Json, TSharedPtr<FJsonValue>& Value)
{
    return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Value) && Value.IsValid();
}
inline bool Description(const TSharedPtr<FJsonValue>& Value, bool bAllowNull = false)
{
    return Value && (Value->Type == EJson::String || Value->Type == EJson::Object || Value->Type == EJson::Array || (bAllowNull && Value->Type == EJson::Null));
}
inline FString TypeName(EJWNU_TypeSafeQuestionType Type)
{
    switch (Type)
    {
    case EJWNU_TypeSafeQuestionType::Choice: return TEXT("choice");
    case EJWNU_TypeSafeQuestionType::Score: return TEXT("score");
    case EJWNU_TypeSafeQuestionType::Noul: return TEXT("noul");
    default: return TEXT("");
    }
}
inline bool MakeQuestion(const FJWNU_TypeSafeQuestion& Q, TSharedPtr<FJsonObject>& Object)
{
    if (Q.Id.IsEmpty() || TypeName(Q.Type).IsEmpty()) { return false; }
    Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("type"), TypeName(Q.Type));
    TSharedPtr<FJsonValue> Instruction;
    if (!Q.InstructionsJson.IsEmpty())
    {
        if (!ReadValue(Q.InstructionsJson, Instruction) || !Description(Instruction)) { return false; }
    }
    else
    {
        if (Q.Instructions.TrimStartAndEnd().IsEmpty()) { return false; }
        Instruction = MakeShared<FJsonValueString>(Q.Instructions);
    }
    Object->SetField(TEXT("instructions"), Instruction);
    TSharedPtr<FJsonValue> Criteria;
    if (!Q.CriteriaJson.IsEmpty())
    {
        if (!ReadValue(Q.CriteriaJson, Criteria)) { return false; }
    }
    else if (Q.Type == EJWNU_TypeSafeQuestionType::Score)
    {
        TArray<TSharedPtr<FJsonValue>> Levels;
        for (const FString& Level : Q.ScoreLevels) { Levels.Add(MakeShared<FJsonValueString>(Level)); }
        Criteria = MakeShared<FJsonValueArray>(Levels);
    }
    else
    {
        auto Map = MakeShared<FJsonObject>();
        if (Q.Type == EJWNU_TypeSafeQuestionType::Choice)
        { for (const auto& Pair : Q.ChoiceOptions) { Map->SetStringField(Pair.Key, Pair.Value); } }
        else
        {
            if (!Q.YesDescription.IsEmpty()) { Map->SetStringField(TEXT("true"), Q.YesDescription); }
            if (!Q.NoDescription.IsEmpty()) { Map->SetStringField(TEXT("false"), Q.NoDescription); }
        }
        Criteria = MakeShared<FJsonValueObject>(Map);
    }
    if (Q.Type == EJWNU_TypeSafeQuestionType::Score)
    {
        const TArray<TSharedPtr<FJsonValue>>* Levels = nullptr;
        if (!Criteria->TryGetArray(Levels) || Levels->Num() < 2 || Levels->Num() > 10) { return false; }
        for (const auto& Level : *Levels) { if (!Description(Level)) { return false; } }
    }
    else
    {
        const TSharedPtr<FJsonObject>* Map = nullptr;
        if (!Criteria->TryGetObject(Map)) { return false; }
        if (Q.Type == EJWNU_TypeSafeQuestionType::Choice && ((*Map)->Values.IsEmpty() || (*Map)->Values.Num() > 255)) { return false; }
        for (const auto& Pair : (*Map)->Values)
        {
            if (Pair.Key.IsEmpty() || !Description(Pair.Value, Q.Type == EJWNU_TypeSafeQuestionType::Choice)) { return false; }
            if (Q.Type == EJWNU_TypeSafeQuestionType::Noul && Pair.Key != TEXT("true") && Pair.Key != TEXT("false")) { return false; }
        }
        if (Q.Type == EJWNU_TypeSafeQuestionType::Noul && (*Map)->Values.IsEmpty()) { return true; }
    }
    Object->SetField(TEXT("criteria"), Criteria);
    return true;
}
inline bool Number(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double& Value, double Min, double Max)
{
    const auto Field = Object->TryGetField(Key);
    return Field && Field->Type == EJson::Number && Field->TryGetNumber(Value) && FMath::IsFinite(Value) && Value >= Min && Value <= Max;
}
inline bool Distribution(const TSharedPtr<FJsonObject>& Answer, const TSet<FString>& Expected, TMap<FString, double>& Out)
{
    const TSharedPtr<FJsonObject>* Map = nullptr;
    if (!Answer->TryGetObjectField(TEXT("probabilities"), Map) || (*Map)->Values.Num() != Expected.Num()) { return false; }
    double Sum = 0;
    for (const FString& Key : Expected)
    {
        double Value;
        if (!Number(*Map, *Key, Value, 0, 1)) { return false; }
        Out.Add(Key, Value); Sum += Value;
    }
    return FMath::Abs(Sum - 1.) <= .002;
}
}
