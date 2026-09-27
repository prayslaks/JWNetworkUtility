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
    /** 음성 대화 세션을 연결할 절대 WebSocket URL 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString Endpoint = TEXT("wss://api.openai.com/v1/live/sessions");
    /** session.start에 전달할 음성 대화 모델 이름 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString Model = TEXT("gpt-live-1");
    /** 대화 전체에 적용할 초기 지침 필드. 사용자 발화와 구분한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live", meta=(MultiLine=true)) FString Instructions = TEXT("한국어로 간결하게 대화하세요.");
    /** 서버가 출력 음성 합성에 사용할 목소리 이름 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString Voice = TEXT("marin");
    /** Responses 백엔드 설정에 전달할 모델 이름 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString BackendModel = TEXT("gpt-5.6-luna");
    /** PCM16 mono little-endian. 16000 또는 24000만 지원하는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") int32 SampleRate = 24000;
    /** 연결의 OpenAI-Safety-Identifier 헤더에 넣을 선택적 식별자 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live") FString SafetyIdentifier;
    /** 연결 요청부터 session.started 수신까지 허용할 전체 시간(초) 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live", meta=(ClampMin="1")) float StartTimeoutSeconds = 20.f;
    /** Close 요청 후 session.closed를 기다릴 최대 시간(초) 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Live", meta=(ClampMin="1")) float CloseTimeoutSeconds = 15.f;
};

/** 서버가 보낸 발화 조각이며 시간은 세션 시작 기준 밀리초다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAILiveTranscript
{
    GENERATED_BODY()
    /** 사용자 입력 자막이면 true, 모델 출력 자막이면 false인 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") bool bInput = false;
    /** 이 이벤트에서 새로 도착한 자막 추가분 필드. 누적 문장이 아니다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString Delta;
    /** 자막 구간 시작을 세션 시작 기준 밀리초로 나타내는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") double StartMilliseconds = 0;
    /** 자막 구간 끝을 세션 시작 기준 밀리초로 나타내는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") double EndMilliseconds = 0;
};

/** 로컬·전송·Provider 오류다. 비치명적 명령 오류는 세션을 종료하지 않는다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAILiveError
{
    GENERATED_BODY()
    /** 공급자 또는 로컬 처리에서 지정한 오류 코드 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString Code;
    /** 실패 원인을 설명하는 진단 메시지 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString Message;
    /** 서버 오류와 대응하는 클라이언트 이벤트 식별자 필드. 없는 경우 비어 있다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") FString ClientEventId;
    /** 세션 전체를 실패로 종료해야 하는 오류인지 나타내는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") bool bFatal = false;
};

/** 최종 종료 정보다. bFinalized가 false이면 최종 사용량을 받지 못했다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAILiveClose
{
    GENERATED_BODY()
    /** 서버의 session.closed를 받아 최종 사용량이 확정되었는지 나타내는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") bool bFinalized = false;
    /** 서버가 마지막으로 보고한 사용 시간(초) 필드. bFinalized가 false면 최종값을 보장하지 않는다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Live") double UsageSeconds = 0;
    /** 서버 또는 로컬 종료 처리에서 제공한 원인 문자열 필드. */
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
