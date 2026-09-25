// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"

namespace JWNU::AudioIO
{
/** 20ms 청크에 정수 개 샘플을 담을 수 있는 일반 음성·장치 표본율이다. */
inline bool IsSupportedSampleRate(int32 Rate)
{
    return Rate == 8000 || Rate == 16000 || Rate == 22050 || Rate == 24000
        || Rate == 32000 || Rate == 44100 || Rate == 48000 || Rate == 96000;
}
/** PCM16 mono little-endian의 RMS를 0~1 범위로 계산한다. */
inline float RMS(const TArray<uint8>& Bytes)
{
    if (Bytes.IsEmpty() || Bytes.Num() % 2) { return 0.f; }
    double Sum = 0;
    for (int32 Offset = 0; Offset < Bytes.Num(); Offset += 2)
    {
        const int32 Bits = static_cast<int32>(Bytes[Offset]) | (static_cast<int32>(Bytes[Offset + 1]) << 8);
        const double Sample = (Bits >= 32768 ? Bits - 65536 : Bits) / 32768.;
        Sum += Sample * Sample;
    }
    return static_cast<float>(FMath::Sqrt(Sum / (Bytes.Num() / 2)));
}
}
