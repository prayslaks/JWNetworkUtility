// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_AudioTypes.generated.h"

/** Provider에 종속되지 않는 로컬 오디오 오류다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYAUDIO_API FJWNU_AudioError
{
    GENERATED_BODY()
    /** 캡처·재생 실패 종류를 식별하는 오류 코드 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") FString Code;
    /** 장치 설정·PCM 형식·버퍼 문제를 설명하는 진단 메시지 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") FString Message;
    /** 현재 오디오 작업을 계속할 수 없는 오류인지 나타내는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") bool bFatal = false;
};

/** 현재 프로세스에서 열 수 있는 마이크 장치 정보다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYAUDIO_API FJWNU_AudioCaptureDevice
{
    GENERATED_BODY()
    /** StartCapture에 전달할 장치 인덱스 필드. -1은 기본 입력 장치를 선택한다. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") int32 DeviceIndex = -1;
    /** 사용자에게 표시할 운영체제 입력 장치 이름 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") FString Name;
    /** 장치가 제공하는 입력 채널 수 필드. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") int32 Channels = 0;
    /** 장치가 선호하는 입력 표본율(Hz) 필드. 전송용 PCM 표본율과 다를 수 있다. */
    UPROPERTY(BlueprintReadOnly, Category="JWNU|Audio") int32 PreferredSampleRate = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_PCMReceivedBP, const TArray<uint8>&, PCM16);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_PCMReceivedNative, const TArray<uint8>&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_AudioErrorBP, const FJWNU_AudioError&, Error);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_AudioErrorNative, const FJWNU_AudioError&);
