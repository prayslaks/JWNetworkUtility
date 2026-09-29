// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_SystemOneCodec.h"
#include "JWNU_SystemOneJson.h"

bool FJWNU_SystemOneCodec::BuildRequest(const FJWNU_SystemOneState& State, const TArray<FJWNU_SystemOneQuestion>& Questions, const FString& Model, FString& Body, FString& Error)
{
    using namespace JWNU::SystemOneJson;
    Body.Reset(); Error = TEXT("Invalid state, model, question ID, instructions or criteria.");
    if (Model.TrimStartAndEnd().IsEmpty() || Questions.IsEmpty()) { return false; }
    TSharedPtr<FJsonValue> StateValue;
    if (State.Format == EJWNU_SystemOneStateFormat::Text) { StateValue = MakeShared<FJsonValueString>(State.Value); }
    else if (State.Format == EJWNU_SystemOneStateFormat::Json)
    { if (!ReadValue(State.Value, StateValue) || !Description(StateValue)) { return false; } }
    else { return false; }
    auto Root = MakeShared<FJsonObject>();
    auto Map = MakeShared<FJsonObject>();
    for (const auto& Q : Questions)
    {
        TSharedPtr<FJsonObject> Object;
        if (Map->HasField(Q.Id) || !MakeQuestion(Q, Object)) { return false; }
        Map->SetObjectField(Q.Id, Object);
    }
    Root->SetStringField(TEXT("model"), Model);
    Root->SetField(TEXT("state"), StateValue);
    Root->SetObjectField(TEXT("questions"), Map);
    if (!FJsonSerializer::Serialize(Root, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Body))) { return false; }
    Error.Reset(); return true;
}

bool FJWNU_SystemOneCodec::ParseResponse(const FString& Body, const TArray<FJWNU_SystemOneQuestion>& Questions, FJWNU_SystemOneResult& Result, FString& Error)
{
    using namespace JWNU::SystemOneJson;
    Result = {}; Error = TEXT("Response does not match the submitted System One questions or numeric contract.");
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root) || !Root || Questions.IsEmpty()) { return false; }
    FJWNU_SystemOneResult Parsed;
    const TSharedPtr<FJsonObject>* Answers = nullptr;
    const TSharedPtr<FJsonObject>* Usage = nullptr;
    if (!Root->TryGetStringField(TEXT("model"), Parsed.Model) || Parsed.Model.IsEmpty()
        || !Root->TryGetObjectField(TEXT("answers"), Answers) || (*Answers)->Values.Num() != Questions.Num()
        || !Root->TryGetObjectField(TEXT("usage"), Usage)) { return false; }
    double Input, Output;
    if (!Number(*Usage, TEXT("input_tokens"), Input, 0, 9007199254740991.)
        || !Number(*Usage, TEXT("output_tokens"), Output, 0, 9007199254740991.)
        || FMath::FloorToDouble(Input) != Input || FMath::FloorToDouble(Output) != Output) { return false; }
    // 추가 메타데이터는 선택적이다. 명시적으로 제공한 값의 타입은 검증한다.
    if ((Root->HasField(TEXT("id")) && !Root->TryGetStringField(TEXT("id"), Parsed.RequestId))
        || (Root->HasField(TEXT("provider")) && !Root->TryGetStringField(TEXT("provider"), Parsed.Provider))) { return false; }
    const auto CostField = (*Usage)->TryGetField(TEXT("cost"));
    if (CostField && CostField->Type != EJson::Null)
    {
        if (!Number(*Usage, TEXT("cost"), Parsed.Cost, 0, TNumericLimits<double>::Max())) { return false; }
        Parsed.bHasCost = true;
    }
    Parsed.InputTokens = static_cast<int64>(Input); Parsed.OutputTokens = static_cast<int64>(Output);
    TSet<FString> Seen;
    for (const auto& Q : Questions)
    {
        TSharedPtr<FJsonObject> Definition;
        const TSharedPtr<FJsonObject>* Answer = nullptr;
        FString Type;
        if (Seen.Contains(Q.Id) || !MakeQuestion(Q, Definition) || !(*Answers)->TryGetObjectField(Q.Id, Answer)
            || !(*Answer)->TryGetStringField(TEXT("type"), Type) || Type != TypeName(Q.Type)) { return false; }
        Seen.Add(Q.Id);
        if (Q.Type == EJWNU_SystemOneQuestionType::Noul)
        {
            FJWNU_SystemOneNoulAnswer Noul;
            if (!Number(*Answer, TEXT("noul"), Noul.Noul, 0, 1)) { return false; }
            Parsed.Nouls.Add(Q.Id, Noul);
        }
        else if (Q.Type == EJWNU_SystemOneQuestionType::Choice)
        {
            FJWNU_SystemOneChoiceAnswer Choice;
            TSet<FString> Keys;
            for (const auto& Pair : Definition->GetObjectField(TEXT("criteria"))->Values) { Keys.Add(Pair.Key); }
            if (!(*Answer)->TryGetStringField(TEXT("choice"), Choice.Choice) || !Keys.Contains(Choice.Choice)
                || !Number(*Answer, TEXT("confidence"), Choice.Confidence, 0, 1)
                || !Distribution(*Answer, Keys, Choice.Probabilities)) { return false; }
            const double Selected = Choice.Probabilities.FindChecked(Choice.Choice);
            for (const auto& Pair : Choice.Probabilities) { if (Pair.Value > Selected + .00001) { return false; } }
            Parsed.Choices.Add(Q.Id, MoveTemp(Choice));
        }
        else
        {
            FJWNU_SystemOneScoreAnswer Score;
            TSet<FString> Keys;
            const int32 Levels = Definition->GetArrayField(TEXT("criteria")).Num();
            for (int32 Index = 0; Index < Levels; ++Index) { Keys.Add(FString::FromInt(Index)); }
            const TSharedPtr<FJsonObject>* Legend = nullptr;
            if (!Number(*Answer, TEXT("score"), Score.Score, 0, Levels - 1)
                || !Number(*Answer, TEXT("confidence"), Score.Confidence, 0, 1)
                || !Distribution(*Answer, Keys, Score.Probabilities)
                || !(*Answer)->TryGetObjectField(TEXT("legend"), Legend) || (*Legend)->Values.Num() != Levels) { return false; }
            for (const FString& Key : Keys)
            {
                FString Description;
                if (!(*Legend)->TryGetStringField(Key, Description)) { return false; }
                Score.Legend.Add(Key, MoveTemp(Description));
            }
            Parsed.Scores.Add(Q.Id, MoveTemp(Score));
        }
    }
    Result = MoveTemp(Parsed); Error.Reset(); return true;
}
