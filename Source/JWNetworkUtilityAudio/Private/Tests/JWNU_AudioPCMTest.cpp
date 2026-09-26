// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_PCMConversion.h"
#include "JWNU_AudioPCM.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS && !UE_SERVER
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJWNU_AudioPCMTest, "JWNetworkUtility.Audio.PCM",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FJWNU_AudioPCMTest::RunTest(const FString& Parameters)
{
    TArray<float> Stereo;
    for (int32 Index = 0; Index < 48000; ++Index)
    {
        const float Sample = .5f * FMath::Sin(2.f * PI * 440.f * Index / 48000.f);
        Stereo.Add(Sample); Stereo.Add(Sample);
    }
    JWNU::AudioIO::FPCMConverter Whole, Chunked;
    TArray<uint8> One, Many;
    Whole.Convert(Stereo.GetData(), 48000, 2, 48000, One);
    for (int32 Offset = 0; Offset < 48000; Offset += 997)
    { Chunked.Convert(Stereo.GetData() + Offset * 2, FMath::Min(997, 48000 - Offset), 2, 48000, Many); }
    TestTrue(TEXT("48k stereo to 24k mono duration"), FMath::Abs(One.Num() - 48000) <= 2);
    TestTrue(TEXT("Callback boundaries preserve PCM"), One == Many);
    JWNU::AudioIO::FPCMConverter Same;
    const float Limits[] = { -2.f, 2.f, 0.f, -.5f, .5f };
    TArray<uint8> Bytes;
    Same.Convert(Limits, 5, 1, 24000, Bytes);
    TestEqual(TEXT("Same-rate length"), Bytes.Num(), 10);
    if (Bytes.Num() >= 4)
    {
        TestEqual(TEXT("Negative PCM16 LE low byte"), Bytes[0], static_cast<uint8>(0));
        TestEqual(TEXT("Negative PCM16 LE high byte"), Bytes[1], static_cast<uint8>(0x80));
        TestEqual(TEXT("Positive PCM16 LE low byte"), Bytes[2], static_cast<uint8>(0xff));
        TestEqual(TEXT("Positive PCM16 LE high byte"), Bytes[3], static_cast<uint8>(0x7f));
    }
    JWNU::AudioIO::FPCMConverter Down;
    Down.TargetRate = 16000;
    Bytes.Reset(); Down.Convert(Stereo.GetData(), 48000, 2, 48000, Bytes);
    TestTrue(TEXT("16k output duration"), FMath::Abs(Bytes.Num() - 32000) <= 2);
    JWNU::AudioIO::FCaptureBuffer Capture;
    Capture.Push(Stereo.GetData(), 480, 2, 48000, true);
    TestFalse(TEXT("Initial WASAPI discontinuity is accepted"), Capture.bOverflow);
    TestFalse(TEXT("Initial samples preserved"), Capture.Bytes.IsEmpty());
    Capture.Push(Stereo.GetData(), 480, 2, 48000, true);
    TestTrue(TEXT("Mid-stream discontinuity is reported"), Capture.bOverflow);
    JWNU::AudioIO::FCaptureBuffer Bounded;
    Bounded.Push(Stereo.GetData(), 48000, 2, 48000, false);
    TestTrue(TEXT("Capture backlog bounded"), Bounded.bOverflow);
    TestTrue(TEXT("Capture overflow releases backlog"), Bounded.Bytes.IsEmpty());
    for (int32 Rate : {8000, 16000, 22050, 24000, 32000, 44100, 48000, 96000})
    {
        TestTrue(TEXT("Generic sample rate supported"), JWNU::AudioIO::IsSupportedSampleRate(Rate));
        JWNU::AudioIO::FPCMConverter Converter;
        Converter.TargetRate = Rate;
        TArray<uint8> Converted;
        Converter.Convert(Stereo.GetData(), 48000, 2, 48000, Converted);
        TestTrue(FString::Printf(TEXT("Output duration at %d Hz"), Rate), FMath::Abs(Converted.Num() - Rate * 2) <= 4);
    }
    TestFalse(TEXT("Unknown sample rate rejected"), JWNU::AudioIO::IsSupportedSampleRate(12345));
    TestEqual(TEXT("RMS silence"), JWNU::AudioIO::RMS({0, 0, 0, 0}), 0.f);
    TestEqual(TEXT("RMS half scale"), JWNU::AudioIO::RMS({0, 0x40, 0, 0xc0}), .5f);
    TestEqual(TEXT("RMS invalid half sample"), JWNU::AudioIO::RMS({0}), 0.f);
    return true;
}
#endif
