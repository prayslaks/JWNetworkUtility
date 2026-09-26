// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_TypeSafeCodec.h"
#include "JWNU_BFL_TypeSafe.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_TypeSafeCodecTest, "JWNetworkUtility.TypeSafe.Codec",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_TypeSafeCodecTest::RunTest(const FString& Parameters)
{
    FJWNU_TypeSafeState State; State.Format = EJWNU_TypeSafeStateFormat::Json; State.Value = TEXT("{\"message\":\"편대 복귀 🙂\"}");
    TArray<FJWNU_TypeSafeQuestion> Questions {
        UJWNU_BFL_TypeSafe::MakeChoiceQuestion(TEXT("action"), TEXT("명령 종류"), {{TEXT("return"), TEXT("복귀")}, {TEXT("none"), TEXT("명령 아님")}}),
        UJWNU_BFL_TypeSafe::MakeScoreQuestion(TEXT("priority"), TEXT("긴급도"), {TEXT("낮음"), TEXT("높음")}),
        UJWNU_BFL_TypeSafe::MakeNoulQuestion(TEXT("is_command"), TEXT("실행 명령인가?"), {}) };
    FString Body, Error;
    TestTrue(TEXT("Mixed request built"), FJWNU_TypeSafeCodec::BuildRequest(State, Questions, TEXT("jev-latest"), Body, Error));
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    TestEqual(TEXT("JSON state is not double encoded"), Root->GetObjectField(TEXT("state"))->GetStringField(TEXT("message")), FString(TEXT("편대 복귀 🙂")));
    TestFalse(TEXT("Empty Noul criteria omitted"), Root->GetObjectField(TEXT("questions"))->GetObjectField(TEXT("is_command"))->HasField(TEXT("criteria")));
    FJWNU_TypeSafeNoulCriteria NoulCriteria;
    NoulCriteria.TrueDescription = TEXT("이전에 문의하거나 요청한 적이 있음을 명시함");
    NoulCriteria.FalseDescription = TEXT("이전 문의를 나타내는 정보가 없음");
    auto WithCriteria = Questions;
    WithCriteria[2] = UJWNU_BFL_TypeSafe::MakeNoulQuestion(TEXT("is_command"), TEXT("이전 문의인가?"), NoulCriteria);
    TestTrue(TEXT("Noul description criteria accepted"), FJWNU_TypeSafeCodec::BuildRequest(State, WithCriteria, TEXT("jev-latest"), Body, Error));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    auto NoulJson = Root->GetObjectField(TEXT("questions"))->GetObjectField(TEXT("is_command"))->GetObjectField(TEXT("criteria"));
    TestEqual(TEXT("Noul true description serialized"), NoulJson->GetStringField(TEXT("true")), NoulCriteria.TrueDescription);
    TestEqual(TEXT("Noul false description serialized"), NoulJson->GetStringField(TEXT("false")), NoulCriteria.FalseDescription);
    TestEqual(TEXT("Only true/false keys"), NoulJson->Values.Num(), 2);
    NoulCriteria.FalseDescription.Reset();
    WithCriteria[2] = UJWNU_BFL_TypeSafe::MakeNoulQuestion(TEXT("is_command"), TEXT("이전 문의인가?"), NoulCriteria);
    TestTrue(TEXT("One-sided criteria accepted"), FJWNU_TypeSafeCodec::BuildRequest(State, WithCriteria, TEXT("jev-latest"), Body, Error));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    NoulJson = Root->GetObjectField(TEXT("questions"))->GetObjectField(TEXT("is_command"))->GetObjectField(TEXT("criteria"));
    TestFalse(TEXT("Empty false description omitted"), NoulJson->HasField(TEXT("false")));
    WithCriteria[2].CriteriaJson = TEXT("{\"false\":\"고급 기준\"}");
    TestTrue(TEXT("Advanced JSON criteria still overrides helper"), FJWNU_TypeSafeCodec::BuildRequest(State, WithCriteria, TEXT("jev-latest"), Body, Error));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    NoulJson = Root->GetObjectField(TEXT("questions"))->GetObjectField(TEXT("is_command"))->GetObjectField(TEXT("criteria"));
    TestEqual(TEXT("JSON override preserved"), NoulJson->GetStringField(TEXT("false")), FString(TEXT("고급 기준")));
    TestFalse(TEXT("Overridden true not merged"), NoulJson->HasField(TEXT("true")));
    State.Format = EJWNU_TypeSafeStateFormat::Text;
    TestTrue(TEXT("JSON-looking text accepted"), FJWNU_TypeSafeCodec::BuildRequest(State, Questions, TEXT("jev-latest"), Body, Error));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    TestEqual(TEXT("Text remains literal"), Root->GetStringField(TEXT("state")), State.Value);
    auto Invalid = Questions; Invalid.Add(Questions[0]);
    TestFalse(TEXT("Duplicate IDs rejected"), FJWNU_TypeSafeCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid = Questions; Invalid[1].ScoreLevels = {TEXT("only one")};
    TestFalse(TEXT("Single score level rejected"), FJWNU_TypeSafeCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid[1].ScoreLevels.SetNum(11);
    TestFalse(TEXT("Eleven score levels rejected"), FJWNU_TypeSafeCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid = Questions; Invalid[0].ChoiceOptions.Reset();
    for (int32 I = 0; I < 256; ++I) { Invalid[0].ChoiceOptions.Add(FString::FromInt(I), TEXT("option")); }
    TestFalse(TEXT("256 choices rejected"), FJWNU_TypeSafeCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid = Questions; Invalid[0].InstructionsJson = TEXT("{\"question\":\"명령?\"}"); Invalid[0].CriteriaJson = TEXT("{\"return\":null,\"none\":{\"meaning\":\"없음\"}}");
    TestTrue(TEXT("Structured instructions and null/object criteria"), FJWNU_TypeSafeCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid[2].CriteriaJson = TEXT("{\"yes\":\"invalid key\"}");
    TestFalse(TEXT("Noul keys must be true/false"), FJWNU_TypeSafeCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    State.Format = EJWNU_TypeSafeStateFormat::Json; State.Value = TEXT("42");
    TestFalse(TEXT("Scalar numeric state rejected"), FJWNU_TypeSafeCodec::BuildRequest(State, Questions, TEXT("jev-latest"), Body, Error));
    const FString Valid = TEXT("{\"model\":\"jev-1.13.0\",\"answers\":{\"action\":{\"type\":\"choice\",\"choice\":\"return\",\"probabilities\":{\"return\":0.8,\"none\":0.2},\"confidence\":0.6},\"priority\":{\"type\":\"score\",\"score\":0.75,\"legend\":{\"0\":\"낮음\",\"1\":\"높음\"},\"probabilities\":{\"0\":0.25,\"1\":0.75},\"confidence\":0.6},\"is_command\":{\"type\":\"noul\",\"noul\":0.85}},\"usage\":{\"input_tokens\":42,\"output_tokens\":9}}");
    FJWNU_TypeSafeResult Result;
    TestTrue(TEXT("Typed response accepted"), FJWNU_TypeSafeCodec::ParseResponse(Valid, Questions, Result, Error));
    TestEqual(TEXT("Score is fractional"), Result.Scores.FindChecked(TEXT("priority")).Score, .75);
    TestEqual(TEXT("Noul remains probability"), Result.Nouls.FindChecked(TEXT("is_command")).Noul, .85);
    const TArray<TPair<FString, FString>> Mutations {
        {TEXT("\"noul\":0.85"), TEXT("\"noul\":1.5")},
        {TEXT("\"noul\":0.85"), TEXT("\"noul\":\"0.85\"")},
        {TEXT("\"type\":\"noul\""), TEXT("\"type\":\"choice\"")},
        {TEXT("\"is_command\":"), TEXT("\"unexpected\":")},
        {TEXT("\"choice\":\"return\""), TEXT("\"choice\":\"unknown\"")},
        {TEXT("\"none\":0.2"), TEXT("\"none\":0.5")},
        {TEXT("\"score\":0.75"), TEXT("\"score\":2")},
        {TEXT("\"confidence\":0.6"), TEXT("\"confidence\":-1")},
        {TEXT("\"input_tokens\":42"), TEXT("\"input_tokens\":1.5")},
        {TEXT("\"output_tokens\":9"), TEXT("\"output_tokens\":-1")} };
    for (const auto& Pair : Mutations)
    {
        TestFalse(TEXT("Malformed contract rejected: ") + Pair.Key, FJWNU_TypeSafeCodec::ParseResponse(Valid.Replace(*Pair.Key, *Pair.Value), Questions, Result, Error));
        TestTrue(TEXT("Partial answers not exposed"), Result.Choices.IsEmpty() && Result.Scores.IsEmpty() && Result.Nouls.IsEmpty());
    }
    TestFalse(TEXT("Invalid JSON rejected"), FJWNU_TypeSafeCodec::ParseResponse(TEXT("garbage"), Questions, Result, Error));
    return true;
}
#endif
