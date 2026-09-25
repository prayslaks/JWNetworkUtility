// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "JWNU_AudioTypes.generated.h"

/** Provider에 종속되지 않는 로컬 오디오 오류다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYAUDIO_API FJWNU_AudioError
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") FString Code;
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") FString Message;
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") bool bFatal = false;
};

/** 현재 프로세스에서 열 수 있는 마이크 장치 정보다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYAUDIO_API FJWNU_AudioCaptureDevice
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") int32 DeviceIndex = -1;
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") FString Name;
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") int32 Channels = 0;
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") int32 PreferredSampleRate = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_PCMReceivedBP, const TArray<uint8>&, PCM16);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_PCMReceivedNative, const TArray<uint8>&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_AudioErrorBP, const FJWNU_AudioError&, Error);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_AudioErrorNative, const FJWNU_AudioError&);
