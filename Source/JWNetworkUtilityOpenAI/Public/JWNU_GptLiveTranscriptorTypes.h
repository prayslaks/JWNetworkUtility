// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNU_GptLiveTranscriptorTypes.generated.h"

/** 발화별 누적 부분문 또는 최종문. Text 전체로 자막을 교체한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_GptLiveTranscript
{
	GENERATED_BODY()
	/** 서버가 부여한 발화 식별자. Start마다 기록을 새로 시작한다. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|OpenAI|Transcription") FString UtteranceId;
	/** delta 누적 스냅샷이며 최종 수신 시 최종문으로 교체된다. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|OpenAI|Transcription") FString Text;
	/** false는 즉시 전달하는 부분문, true는 입력 순서대로 전달하는 최종문이다. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|OpenAI|Transcription") bool bFinal = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJWNU_GptLiveSignalBP);
DECLARE_MULTICAST_DELEGATE(FJWNU_GptLiveSignalNative);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_GptLiveTranscriptBP, const FJWNU_GptLiveTranscript&, Transcript);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_GptLiveTranscriptNative, const FJWNU_GptLiveTranscript&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_GptLiveErrorBP, const FString&, Message);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_GptLiveErrorNative, const FString&);
