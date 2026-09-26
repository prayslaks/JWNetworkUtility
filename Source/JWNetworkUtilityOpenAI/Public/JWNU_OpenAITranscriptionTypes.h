// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_OpenAITranscriptionTypes.generated.h"

/** 확인된 전사 모델 이름. 목록 밖 이름도 그대로 서버에 전달한다. */
namespace JWNU::OpenAITranscription::Models
{
inline const TCHAR* LiveTranscribe = TEXT("gpt-live-transcribe");
inline const TCHAR* Transcribe = TEXT("gpt-transcribe");
inline const TCHAR* RealtimeWhisper = TEXT("gpt-realtime-whisper");
}

UENUM(BlueprintType)
enum class EJWNU_OpenAITranscriptionDelay : uint8 { Default, Minimal, Low, Medium, High, XHigh };

UENUM(BlueprintType)
enum class EJWNU_OpenAITranscriptionState : uint8 { Idle, Connecting, Starting, Ready, Closing, Closed, Failed };

/**
 * PCM16 mono 24kHz 전용. 서버 VAD 대신 호출자가 CommitInputAudio로 발화를 확정한다.
 * 비어 있지 않은 필드만 전송한다. 모델별 필드 지원 여부는 서버가 판정하며 미지원 조합은 시작 시 서버 오류로 통지된다.
 */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscriptionOptions
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") FString Endpoint = TEXT("wss://api.openai.com/v1/realtime?intent=transcription");
    /** 서버 모델 이름. 기본값은 공식 권장 모델이며 새 모델은 이름만 바꿔 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") FString Model =JWNU::OpenAITranscription::Models::LiveTranscribe;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") EJWNU_OpenAITranscriptionDelay Delay = EJWNU_OpenAITranscriptionDelay::Default;
    /** 단일 언어 힌트(`language`). 빈 값은 자동 감지다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") FString Language;
    /** 언어 힌트 목록(`languages`). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") TArray<FString> Languages;
    /** 녹음 상황을 설명하는 문맥(`prompt`). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription", meta=(MultiLine=true)) FString Prompt;
    /** 용어 힌트(`keywords`). 개행과 < > 문자는 허용하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") TArray<FString> Keywords;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") float StartTimeoutSeconds = 20.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") float CloseTimeoutSeconds = 15.f;
};

/** Delta는 추가분, 최종 Transcript는 해당 ItemId의 전체 확정문이다. 도착 순서는 발화 순서가 아니다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscript
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString ItemId;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") int32 ContentIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Delta;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Transcript;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") bool bFinal = false;
};

USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscriptionCommit
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString ItemId;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString PreviousItemId;
};

USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscriptionError
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Code;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Message;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString EventId;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString ItemId;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") bool bFatal = false;
};

USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscriptionClose
{
    GENERATED_BODY()
    /** 모든 제출 발화가 성공적으로 확정된 정상 Close에서만 true다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") bool bFinalized = false;
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Reason;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptionReadyBP, const FString&, SessionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptBP, const FJWNU_OpenAITranscript&, Transcript);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptionCommittedBP, const FJWNU_OpenAITranscriptionCommit&, Commit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptionErrorBP, const FJWNU_OpenAITranscriptionError&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptionClosedBP, const FJWNU_OpenAITranscriptionClose&, Info);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FJWNU_TranscriptionRawBP, const FString&, Type, const FString&, Json);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptionReadyNative, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptNative, const FJWNU_OpenAITranscript&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptionCommittedNative, const FJWNU_OpenAITranscriptionCommit&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptionErrorNative, const FJWNU_OpenAITranscriptionError&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptionClosedNative, const FJWNU_OpenAITranscriptionClose&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FJWNU_TranscriptionRawNative, const FString&, const FString&);
