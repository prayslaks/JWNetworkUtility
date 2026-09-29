// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_SystemOneCodec.h"
#include "JWNU_BFL_SystemOne.h"
#include "Misc/AutomationTest.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_SystemOneCodecTest, "JWNetworkUtility.SystemOne.Codec",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_SystemOneCodecTest::RunTest(const FString& Parameters)
{
    FJWNU_SystemOneState State; State.Format = EJWNU_SystemOneStateFormat::Json; State.Value = TEXT("{\"message\":\"편대 복귀 🙂\"}");
    TArray<FJWNU_SystemOneQuestion> Questions {
        UJWNU_BFL_SystemOne::MakeChoiceQuestion(TEXT("action"), TEXT("명령 종류"), {{TEXT("return"), TEXT("복귀")}, {TEXT("none"), TEXT("명령 아님")}}),
        UJWNU_BFL_SystemOne::MakeScoreQuestion(TEXT("priority"), TEXT("긴급도"), {TEXT("낮음"), TEXT("높음")}),
        UJWNU_BFL_SystemOne::MakeNoulQuestion(TEXT("is_command"), TEXT("실행 명령인가?"), {}) };
    FString Body, Error;
    TestTrue(TEXT("Mixed request built"), FJWNU_SystemOneCodec::BuildRequest(State, Questions, TEXT("jev-latest"), Body, Error));
    TSharedPtr<FJsonObject> Root;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    TestEqual(TEXT("JSON state is not double encoded"), Root->GetObjectField(TEXT("state"))->GetStringField(TEXT("message")), FString(TEXT("편대 복귀 🙂")));
    TestFalse(TEXT("Empty Noul criteria omitted"), Root->GetObjectField(TEXT("questions"))->GetObjectField(TEXT("is_command"))->HasField(TEXT("criteria")));
    FJWNU_SystemOneNoulCriteria NoulCriteria;
    NoulCriteria.TrueDescription = TEXT("이전에 문의하거나 요청한 적이 있음을 명시함");
    NoulCriteria.FalseDescription = TEXT("이전 문의를 나타내는 정보가 없음");
    auto WithCriteria = Questions;
    WithCriteria[2] = UJWNU_BFL_SystemOne::MakeNoulQuestion(TEXT("is_command"), TEXT("이전 문의인가?"), NoulCriteria);
    TestTrue(TEXT("Noul description criteria accepted"), FJWNU_SystemOneCodec::BuildRequest(State, WithCriteria, TEXT("jev-latest"), Body, Error));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    auto NoulJson = Root->GetObjectField(TEXT("questions"))->GetObjectField(TEXT("is_command"))->GetObjectField(TEXT("criteria"));
    TestEqual(TEXT("Noul true description serialized"), NoulJson->GetStringField(TEXT("true")), NoulCriteria.TrueDescription);
    TestEqual(TEXT("Noul false description serialized"), NoulJson->GetStringField(TEXT("false")), NoulCriteria.FalseDescription);
    TestEqual(TEXT("Only true/false keys"), NoulJson->Values.Num(), 2);
    NoulCriteria.FalseDescription.Reset();
    WithCriteria[2] = UJWNU_BFL_SystemOne::MakeNoulQuestion(TEXT("is_command"), TEXT("이전 문의인가?"), NoulCriteria);
    TestTrue(TEXT("One-sided criteria accepted"), FJWNU_SystemOneCodec::BuildRequest(State, WithCriteria, TEXT("jev-latest"), Body, Error));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    NoulJson = Root->GetObjectField(TEXT("questions"))->GetObjectField(TEXT("is_command"))->GetObjectField(TEXT("criteria"));
    TestFalse(TEXT("Empty false description omitted"), NoulJson->HasField(TEXT("false")));
    WithCriteria[2].CriteriaJson = TEXT("{\"false\":\"고급 기준\"}");
    TestTrue(TEXT("Advanced JSON criteria still overrides helper"), FJWNU_SystemOneCodec::BuildRequest(State, WithCriteria, TEXT("jev-latest"), Body, Error));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    NoulJson = Root->GetObjectField(TEXT("questions"))->GetObjectField(TEXT("is_command"))->GetObjectField(TEXT("criteria"));
    TestEqual(TEXT("JSON override preserved"), NoulJson->GetStringField(TEXT("false")), FString(TEXT("고급 기준")));
    TestFalse(TEXT("Overridden true not merged"), NoulJson->HasField(TEXT("true")));
    State.Format = EJWNU_SystemOneStateFormat::Text;
    TestTrue(TEXT("JSON-looking text accepted"), FJWNU_SystemOneCodec::BuildRequest(State, Questions, TEXT("jev-latest"), Body, Error));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
    TestEqual(TEXT("Text remains literal"), Root->GetStringField(TEXT("state")), State.Value);
    auto Invalid = Questions; Invalid.Add(Questions[0]);
    TestFalse(TEXT("Duplicate IDs rejected"), FJWNU_SystemOneCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid = Questions; Invalid[1].ScoreLevels = {TEXT("only one")};
    TestFalse(TEXT("Single score level rejected"), FJWNU_SystemOneCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid[1].ScoreLevels.SetNum(11);
    TestFalse(TEXT("Eleven score levels rejected"), FJWNU_SystemOneCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid = Questions; Invalid[0].ChoiceOptions.Reset();
    for (int32 I = 0; I < 256; ++I) { Invalid[0].ChoiceOptions.Add(FString::FromInt(I), TEXT("option")); }
    TestFalse(TEXT("256 choices rejected"), FJWNU_SystemOneCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid = Questions; Invalid[0].InstructionsJson = TEXT("{\"question\":\"명령?\"}"); Invalid[0].CriteriaJson = TEXT("{\"return\":null,\"none\":{\"meaning\":\"없음\"}}");
    TestTrue(TEXT("Structured instructions and null/object criteria"), FJWNU_SystemOneCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    Invalid[2].CriteriaJson = TEXT("{\"yes\":\"invalid key\"}");
    TestFalse(TEXT("Noul keys must be true/false"), FJWNU_SystemOneCodec::BuildRequest(State, Invalid, TEXT("jev-latest"), Body, Error));
    State.Format = EJWNU_SystemOneStateFormat::Json; State.Value = TEXT("42");
    TestFalse(TEXT("Scalar numeric state rejected"), FJWNU_SystemOneCodec::BuildRequest(State, Questions, TEXT("jev-latest"), Body, Error));
    const FString Valid = TEXT("{\"model\":\"jev-1.13.0\",\"answers\":{\"action\":{\"type\":\"choice\",\"choice\":\"return\",\"probabilities\":{\"return\":0.8,\"none\":0.2},\"confidence\":0.6},\"priority\":{\"type\":\"score\",\"score\":0.75,\"legend\":{\"0\":\"낮음\",\"1\":\"높음\"},\"probabilities\":{\"0\":0.25,\"1\":0.75},\"confidence\":0.6},\"is_command\":{\"type\":\"noul\",\"noul\":0.85}},\"usage\":{\"input_tokens\":42,\"output_tokens\":9}}");
    FJWNU_SystemOneResult Result;
    TestTrue(TEXT("Typed response accepted"), FJWNU_SystemOneCodec::ParseResponse(Valid, Questions, Result, Error));
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
        TestFalse(TEXT("Malformed contract rejected: ") + Pair.Key, FJWNU_SystemOneCodec::ParseResponse(Valid.Replace(*Pair.Key, *Pair.Value), Questions, Result, Error));
        TestTrue(TEXT("Partial answers not exposed"), Result.Choices.IsEmpty() && Result.Scores.IsEmpty() && Result.Nouls.IsEmpty());
    }
    TestFalse(TEXT("Invalid JSON rejected"), FJWNU_SystemOneCodec::ParseResponse(TEXT("garbage"), Questions, Result, Error));

    // OpenRouter 메타데이터와 추가 모델은 기존 질문 계약을 변경하지 않는다.
    for (const FString& Model : UJWNU_BFL_SystemOne::GetOpenRouterModelPresets())
    {
        State.Format = EJWNU_SystemOneStateFormat::Text; State.Value = TEXT("편대 복귀");
        TestTrue(TEXT("Model preset serialized"), FJWNU_SystemOneCodec::BuildRequest(State, Questions, Model, Body, Error));
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root);
        TestEqual(TEXT("Exact model ID retained"), Root->GetStringField(TEXT("model")), Model);
        const FString Metadata = Valid.Replace(TEXT("\"model\":\"jev-1.13.0\""), *(FString(TEXT("\"id\":\"req-1\",\"provider\":\"Fixture\",\"model\":\"")) + Model + TEXT("\"")))
            .Replace(TEXT("\"input_tokens\":42"), TEXT("\"cost\":0.00001,\"input_tokens\":42"));
        TestTrue(TEXT("OpenRouter response accepted"), FJWNU_SystemOneCodec::ParseResponse(Metadata, Questions, Result, Error));
        TestEqual(TEXT("Actual model preserved"), Result.Model, Model);
        TestEqual(TEXT("Request ID preserved"), Result.RequestId, FString(TEXT("req-1")));
        TestEqual(TEXT("Actual provider preserved"), Result.Provider, FString(TEXT("Fixture")));
        TestTrue(TEXT("Cost present"), Result.bHasCost && FMath::IsNearlyEqual(Result.Cost, .00001));
    }
    for (const FString& Cost : {FString(TEXT("0")), FString(TEXT("null"))})
    {
        const FString Response = Valid.Replace(TEXT("\"input_tokens\":42"), *(TEXT("\"cost\":") + Cost + TEXT(",\"input_tokens\":42")));
        TestTrue(TEXT("Zero or unknown cost accepted"), FJWNU_SystemOneCodec::ParseResponse(Response, Questions, Result, Error));
        TestEqual(TEXT("Zero is distinct from missing cost"), Result.bHasCost, Cost == TEXT("0"));
    }
    TestTrue(TEXT("Missing metadata remains compatible"), FJWNU_SystemOneCodec::ParseResponse(Valid, Questions, Result, Error));
    TestTrue(TEXT("Missing metadata is reset"), !Result.bHasCost && Result.Cost == 0 && Result.RequestId.IsEmpty() && Result.Provider.IsEmpty());
    for (const FString& Cost : {FString(TEXT("-1")), FString(TEXT("\"0.1\""))})
    {
        const FString Response = Valid.Replace(TEXT("\"input_tokens\":42"), *(TEXT("\"cost\":") + Cost + TEXT(",\"input_tokens\":42")));
        TestFalse(TEXT("Malformed cost rejected"), FJWNU_SystemOneCodec::ParseResponse(Response, Questions, Result, Error));
        TestTrue(TEXT("Malformed response does not expose partial results"), Result.Choices.IsEmpty() && !Result.bHasCost);
    }
    TestEqual(TEXT("New default endpoint"), FJWNU_SystemOneOptions{}.Endpoint, FString(TEXT("https://openrouter.ai/api/alpha/decisions")));
    TestEqual(TEXT("TypeSafe preset endpoint"), UJWNU_BFL_SystemOne::MakeTypeSafeOptions().Endpoint, FString(TEXT("https://api.typesafe.ai/v1/systemone")));
    TestEqual(TEXT("TypeSafe preset model"), UJWNU_BFL_SystemOne::MakeTypeSafeOptions().Model, FString(TEXT("jev-latest")));
    TestEqual(TEXT("Arbitrary future model remains configurable"), UJWNU_BFL_SystemOne::MakeOpenRouterOptions(TEXT("future/model")).Model, FString(TEXT("future/model")));
    return true;
}
#endif
