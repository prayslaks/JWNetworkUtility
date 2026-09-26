// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_OpenAILiveTypes.generated.h"

/** GPT-Live의 연결·시작·종료 상태다. */
UENUM(BlueprintType)
enum class EJWNU_OpenAILiveState : uint8 { Idle, Connecting, Starting, Ready, Closing, Closed, Failed };

/** 세션 시작 설정이다. 인증 키는 직렬화하지 않고 Start 인자로만 전달한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAILiveOptions
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString Endpoint = TEXT("wss://api.openai.com/v1/live/sessions");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString Model = TEXT("gpt-live-1");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live", meta=(MultiLine=true)) FString Instructions = TEXT("한국어로 간결하게 대화하세요.");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString Voice = TEXT("marin");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString BackendModel = TEXT("gpt-5.6-luna");
    /** PCM16 mono little-endian. 16000 또는 24000만 지원하는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") int32 SampleRate = 24000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString SafetyIdentifier;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live", meta=(ClampMin="1")) float StartTimeoutSeconds = 20.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live", meta=(ClampMin="1")) float CloseTimeoutSeconds = 15.f;
};

/** 서버가 보낸 발화 조각이며 시간은 세션 시작 기준 밀리초다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAILiveTranscript
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") bool bInput = false;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString Delta;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") double StartMilliseconds = 0;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") double EndMilliseconds = 0;
};

/** 로컬·전송·Provider 오류다. 비치명적 명령 오류는 세션을 종료하지 않는다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAILiveError
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString Code;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString Message;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString ClientEventId;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") bool bFatal = false;
};

/** 최종 종료 정보다. bFinalized가 false이면 최종 사용량을 받지 못했다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAILiveClose
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") bool bFinalized = false;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") double UsageSeconds = 0;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString Reason;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_LiveReadyBP, const FString&, SessionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_LiveTranscriptBP, const FJWNU_OpenAILiveTranscript&, Transcript);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_LiveAudioBP, const TArray<uint8>&, PCM16);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_LiveErrorBP, const FJWNU_OpenAILiveError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_LiveClosedBP, const FJWNU_OpenAILiveClose&, Info);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_LiveUsageBP, double, Seconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJWNU_LiveRawBP, const FString&, Type, const FString&, Json);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_LiveReadyNative, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_LiveTranscriptNative, const FJWNU_OpenAILiveTranscript&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_LiveAudioNative, const TArray<uint8>&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_LiveErrorNative, const FJWNU_OpenAILiveError&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_LiveClosedNative, const FJWNU_OpenAILiveClose&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_LiveUsageNative, double);
DECLARE_MULTICAST_DELEGATE_TwoParams(FJWNU_LiveRawNative, const FString&, const FString&);
