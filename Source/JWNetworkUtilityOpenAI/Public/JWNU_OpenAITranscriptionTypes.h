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

/** 서버 전사 지연 옵션이다. Default는 delay 필드를 생략해 서버 기본값을 사용한다. */
UENUM(BlueprintType)
enum class EJWNU_OpenAITranscriptionDelay : uint8 { Default, Minimal, Low, Medium, High, XHigh };

/** 연결·설정 확인·입력 준비·최종 결과 대기·종료를 구분하는 전사 세션 상태다. */
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
    /** Realtime 전사 세션을 연결할 절대 WebSocket URL 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") FString Endpoint = TEXT("wss://api.openai.com/v1/realtime?intent=transcription");
    /** 서버 모델 이름. 기본값은 공식 권장 모델이며 새 모델은 이름만 바꿔 사용한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") FString Model =JWNU::OpenAITranscription::Models::LiveTranscribe;
    /** 서버에 요청할 전사 지연 수준 필드. 모델의 지원 여부는 서버가 판정한다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") EJWNU_OpenAITranscriptionDelay Delay = EJWNU_OpenAITranscriptionDelay::Default;
    /** 단일 언어 힌트(`language`). 빈 값은 자동 감지다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") FString Language;
    /** 언어 힌트 목록(`languages`). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") TArray<FString> Languages;
    /** 녹음 상황을 설명하는 문맥(`prompt`). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription", meta=(MultiLine=true)) FString Prompt;
    /** 용어 힌트(`keywords`). 개행과 < > 문자는 허용하지 않는다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") TArray<FString> Keywords;
    /** 연결 요청부터 session.updated 설정 확인까지 허용할 시간(초) 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") float StartTimeoutSeconds = 20.f;
    /** Close 후 commit 응답과 최종 전사를 기다릴 최대 시간(초) 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI|Transcription") float CloseTimeoutSeconds = 15.f;
};

/** Delta는 추가분, 최종 Transcript는 해당 ItemId의 전체 확정문이다. 도착 순서는 발화 순서가 아니다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscript
{
    GENERATED_BODY()
    /** 부분문과 최종문을 같은 발화로 연결할 서버 식별자 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString ItemId;
    /** 아이템 내부 오디오 콘텐츠 인덱스 필드. 현재 세션은 0만 허용한다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") int32 ContentIndex = 0;
    /** bFinal=false일 때 새로 추가할 자막 조각 필드. 누적된 전체 문장이 아니다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Delta;
    /** bFinal=true일 때 해당 발화 자막을 교체할 전체 최종문 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Transcript;
    /** 최종 Transcript인지, 아직 수정 가능한 Delta인지 구분하는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") bool bFinal = false;
};

/** 서버가 입력 버퍼를 발화로 확정했음을 알리고 발화 순서를 연결하는 응답이다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscriptionCommit
{
    GENERATED_BODY()
    /** 이번에 확정된 발화의 서버 식별자 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString ItemId;
    /** 입력 순서상 바로 앞 발화 식별자 필드. 최종 결과 도착 순서와 구분한다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString PreviousItemId;
};

/** 전사 세션 전체 오류와 개별 발화 실패의 원인을 전달하는 진단 정보다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscriptionError
{
    GENERATED_BODY()
    /** 공급자 또는 로컬 처리에서 지정한 오류 코드 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Code;
    /** 실패 원인을 설명하는 진단 메시지 필드. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString Message;
    /** 서버 오류와 대응하는 요청 이벤트 식별자 필드. 제공되지 않으면 비어 있다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString EventId;
    /** 개별 전사 실패가 발생한 발화 식별자 필드. 세션 오류에서는 비어 있을 수 있다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") FString ItemId;
    /** 원시 세션 전체를 실패로 종료하는 오류인지 나타내는 필드. 개별 발화 실패는 false다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") bool bFatal = false;
};

/** 세션이 종료된 원인과 제출한 모든 발화의 정상 확정 여부를 전달한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYOPENAI_API FJWNU_OpenAITranscriptionClose
{
    GENERATED_BODY()
    /** 모든 제출 발화가 성공적으로 확정된 정상 Close에서만 true다. */
    UPROPERTY(BlueprintReadOnly, Category="OpenAI|Transcription") bool bFinalized = false;
    /** 서버 단절·취소·정상 종료 등 종료 원인을 설명하는 문자열 필드. */
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
