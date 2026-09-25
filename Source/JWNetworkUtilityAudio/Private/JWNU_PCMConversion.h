// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#if !UE_SERVER
#include "DSP/RuntimeResampler.h"
#include "Misc/ScopeLock.h"

namespace JWNU::AudioIO
{
/** 장치 콜백 사이 리샘플러 위상을 보존하는 PCM 변환기다. */
struct FPCMConverter
{
    Audio::FRuntimeResampler Resampler{1};
    int32 SourceRate = 0;
    int32 TargetRate = 24000;
    TArray<float> Pending;
    void Convert(const float* Input, int32 Frames, int32 Channels, int32 Rate, TArray<uint8>& Output)
    {
        if (!Input || Frames <= 0 || Channels <= 0 || Rate <= 0) { return; }
        if (SourceRate != Rate)
        {
            SourceRate = Rate; Pending.Reset(); Resampler.Reset(1);
            Resampler.SetFrameRatio(static_cast<float>(Rate) / TargetRate);
        }
        for (int32 Frame = 0; Frame < Frames; ++Frame)
        {
            double Sum = 0;
            for (int32 Channel = 0; Channel < Channels; ++Channel)
            {
                const float Sample = Input[Frame * Channels + Channel];
                Sum += FMath::IsFinite(Sample) ? Sample : 0.f;
            }
            Pending.Add(static_cast<float>(Sum / Channels));
        }
        TArray<float> Resampled;
        Resampled.SetNumUninitialized(FMath::CeilToInt(Pending.Num() * static_cast<double>(TargetRate) / Rate) + 8);
        int32 Consumed = 0, Produced = 0;
        Resampler.ProcessInterleaved(MakeArrayView(Pending), MakeArrayView(Resampled), Consumed, Produced);
        Pending.RemoveAt(0, Consumed, EAllowShrinking::No);
        Output.Reserve(Output.Num() + Produced * 2);
        for (int32 Index = 0; Index < Produced; ++Index)
        {
            const int32 Sample = FMath::Clamp(FMath::RoundToInt(Resampled[Index] * 32768.f), -32768, 32767);
            const uint16 Bits = static_cast<uint16>(static_cast<int16>(Sample));
            Output.Add(static_cast<uint8>(Bits & 0xff)); Output.Add(static_cast<uint8>(Bits >> 8));
        }
    }
};

/** 장치 스레드와 게임 스레드 사이의 상한 있는 버퍼다. */
struct FCaptureBuffer
{
    FCriticalSection Mutex;
    FPCMConverter Converter;
    TArray<uint8> Bytes;
    bool bOverflow = false;
    bool bHasReceived = false;
    double LastCallbackSeconds = FPlatformTime::Seconds();
    void Push(const float* Input, int32 Frames, int32 Channels, int32 Rate, bool bDiscontinuity)
    {
        FScopeLock Lock(&Mutex);
        if (bOverflow || Frames == 0) { return; }
        // WASAPI의 첫 버퍼 discontinuity는 이전 샘플의 유실을 뜻하지 않는다.
        if (!Input || (bDiscontinuity && bHasReceived) || Frames < 0 || Channels <= 0 || Rate <= 0 || Frames > Rate)
        { bOverflow = true; return; }
        bHasReceived = true;
        LastCallbackSeconds = FPlatformTime::Seconds();
        Converter.Convert(Input, Frames, Channels, Rate, Bytes);
        if (Bytes.Num() > Converter.TargetRate / 2) { bOverflow = true; Bytes.Reset(); }
    }
};
}
#endif
