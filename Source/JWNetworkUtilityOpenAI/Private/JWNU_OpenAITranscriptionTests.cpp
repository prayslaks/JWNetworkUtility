// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_OpenAITranscriptionJson.h"
#include "JWNU_OpenAITranscriptionSession.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_OpenAITranscriptionCodecTest, "JWNetworkUtility.OpenAI.Transcription.Codec",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_OpenAITranscriptionCodecTest::RunTest(const FString& Parameters)
{
    using namespace JWNU::OpenAITranscription;
    auto ConfigOf = [](const TSharedRef<FJsonObject>& Root)
    { return Root->GetObjectField(TEXT("session"))->GetObjectField(TEXT("audio"))->GetObjectField(TEXT("input"))->GetObjectField(TEXT("transcription")); };
    FJWNU_OpenAITranscriptionOptions Options;
    TestEqual(TEXT("Recommended default model"), Options.Model, FString(Models::LiveTranscribe));
    TestTrue(TEXT("Default options"), ValidOptions(Options));
    auto Root = UpdateEvent(Options);
    auto Session = Root->GetObjectField(TEXT("session"));
    auto Input = Session->GetObjectField(TEXT("audio"))->GetObjectField(TEXT("input"));
    auto Config = ConfigOf(Root);
    TestEqual(TEXT("GA event"), Root->GetStringField(TEXT("type")), FString(TEXT("session.update")));
    TestEqual(TEXT("Transcription session"), Session->GetStringField(TEXT("type")), FString(TEXT("transcription")));
    TestEqual(TEXT("Model passthrough"), Config->GetStringField(TEXT("model")), FString(Models::LiveTranscribe));
    TestTrue(TEXT("No server VAD"), Input->HasTypedField<EJson::Null>(TEXT("turn_detection")));
    TestFalse(TEXT("No Live delegation"), Session->HasField(TEXT("delegation")));
    for (const TCHAR* Key : {TEXT("delay"), TEXT("language"), TEXT("languages"), TEXT("prompt"), TEXT("keywords")})
    { TestFalse(FString::Printf(TEXT("Unset %s omitted"), Key), Config->HasField(Key)); }

    // 모델별 필드 지원 여부는 서버가 판정하므로 설정한 필드는 모델과 무관하게 그대로 전송한다.
    Options.Model = Models::RealtimeWhisper;
    Options.Delay = EJWNU_OpenAITranscriptionDelay::Low;
    Options.Language = TEXT("ko"); Options.Languages = {TEXT("ko"), TEXT("en")};
    Options.Prompt = TEXT("명령어 문맥"); Options.Keywords = {TEXT("ProjectZK")};
    TestTrue(TEXT("Cross-model fields are not rejected locally"), ValidOptions(Options));
    Config = ConfigOf(UpdateEvent(Options));
    TestEqual(TEXT("Whisper model"), Config->GetStringField(TEXT("model")), FString(Models::RealtimeWhisper));
    TestEqual(TEXT("Delay"), Config->GetStringField(TEXT("delay")), FString(TEXT("low")));
    TestEqual(TEXT("Single language"), Config->GetStringField(TEXT("language")), FString(TEXT("ko")));
    TestEqual(TEXT("Multiple language hints"), Config->GetArrayField(TEXT("languages")).Num(), 2);
    TestEqual(TEXT("Prompt preserved"), Config->GetStringField(TEXT("prompt")), Options.Prompt);
    TestEqual(TEXT("Keyword hint"), Config->GetArrayField(TEXT("keywords"))[0]->AsString(), FString(TEXT("ProjectZK")));

    Options = {}; Options.Model = TEXT("gpt-future-transcribe-2027-01-01");
    TestTrue(TEXT("Unlisted model accepted"), ValidOptions(Options));
    TestEqual(TEXT("Unlisted model passthrough"), ConfigOf(UpdateEvent(Options))->GetStringField(TEXT("model")), Options.Model);

    for (const FString Model : {TEXT(""), TEXT(" "), TEXT("gpt live"), TEXT("gpt\n")})
    { Options = {}; Options.Model = Model; TestFalse(TEXT("Malformed model rejected"), ValidOptions(Options)); }
    for (const FString Keyword : {TEXT("bad\nline"), TEXT("<tag>"), TEXT(" ")})
    { Options = {}; Options.Keywords = {Keyword}; TestFalse(TEXT("Invalid keyword rejected"), ValidOptions(Options)); }
    Options = {}; Options.Languages = {TEXT(" ")};
    TestFalse(TEXT("Blank language rejected"), ValidOptions(Options));
    Options = {}; Options.Language = TEXT("ko\n");
    TestFalse(TEXT("Multiline language rejected"), ValidOptions(Options));
    Options = {}; Options.StartTimeoutSeconds = 0;
    TestFalse(TEXT("Invalid timeout rejected"), ValidOptions(Options));
    Options = {}; Options.Delay = static_cast<EJWNU_OpenAITranscriptionDelay>(255);
    TestFalse(TEXT("Unknown delay enum rejected"), ValidOptions(Options));
    TestEqual(TEXT("Known models exposed"), UJWNU_OpenAITranscriptionSession::GetKnownTranscriptionModels().Num(), 3);
    return true;
}
#endif
