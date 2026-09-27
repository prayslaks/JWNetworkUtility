// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_OpenAITranscriptionTypes.h"
#include "JWNU_GptLiveTranscriptorTypes.h"
#include "JWNU_OpenAITranscriptionTestReceiver.generated.h"
class UJWNU_OpenAITranscriptionSession;
class UJWNU_OpenAITranscriptionComponent;

/** 컴파일한 Blueprint의 전사 이벤트와 Options 미연결 호출을 검증한다. */
UCLASS(Blueprintable)
class UJWNU_OpenAITranscriptionTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    /** Options 미연결 세션 Start 노드의 BP 실행을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Transcription Test") void StartSessionDefaults(UJWNU_OpenAITranscriptionSession* Target);
    /** Options 미연결 환경변수 세션 Start 노드의 BP 컴파일을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Transcription Test") void StartSessionEnvironmentDefaults(UJWNU_OpenAITranscriptionSession* Target);
    /** Options 미연결 컴포넌트 Start 노드의 BP 실행을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Transcription Test") void StartComponentDefaults(UJWNU_OpenAITranscriptionComponent* Target);
    /** Options 미연결 환경변수 컴포넌트 Start 노드의 BP 컴파일을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Transcription Test") void StartComponentEnvironmentDefaults(UJWNU_OpenAITranscriptionComponent* Target);
    /** 원시 전사 이벤트가 생성된 BP 그래프를 통과하는지 검증하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="Transcription Test") void Transcript(const FJWNU_OpenAITranscript& Value);
    virtual void Transcript_Implementation(const FJWNU_OpenAITranscript& Value) { Record(Value); }
    /** 게임 스레드에서 BP를 통과한 원시 전사 이벤트를 저장하는 함수. */
    UFUNCTION(BlueprintCallable, Category="Transcription Test") void Record(const FJWNU_OpenAITranscript& Value) { check(IsInGameThread()); Transcripts.Add(Value); }
    /** 게임 스레드에서 세션 Ready 이벤트 횟수를 기록하는 함수. */
    UFUNCTION() void Ready(const FString& Id) { check(IsInGameThread()); ++ReadyCount; }
    /** 게임 스레드에서 종료 횟수와 마지막 종료 정보를 기록하는 함수. */
    UFUNCTION() void Closed(const FJWNU_OpenAITranscriptionClose& Info) { check(IsInGameThread()); ++ClosedCount; LastClose = Info; }
    /** 게임 스레드에서 오류 횟수와 마지막 오류를 기록하는 함수. */
    UFUNCTION() void Error(const FJWNU_OpenAITranscriptionError& Info) { check(IsInGameThread()); ++ErrorCount; LastError = Info; }
    /** 발화 확정 응답을 저장해 ItemId 연결 순서를 검증하는 함수. */
    UFUNCTION() void Committed(const FJWNU_OpenAITranscriptionCommit& Info) { Commits.Add(Info); }
    TArray<FJWNU_OpenAITranscript> Transcripts;
    TArray<FJWNU_OpenAITranscriptionCommit> Commits;
    int32 ReadyCount = 0, ClosedCount = 0, ErrorCount = 0;
    FJWNU_OpenAITranscriptionClose LastClose;
    FJWNU_OpenAITranscriptionError LastError;
};

/** 누적 자막 전사기의 Blueprint 호환 동적 이벤트를 기록한다. */
UCLASS()
class UJWNU_GptLiveTranscriptorTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    /** 누적 자막 전사기의 동적 이벤트 결과를 순서대로 기록하는 함수. */
    UFUNCTION() void Transcript(const FJWNU_GptLiveTranscript& Value) { Transcripts.Add(Value); }
    /** 누적 자막 전사기의 동적 Ready 전달 횟수를 기록하는 함수. */
    UFUNCTION() void Ready() { ++ReadyCount; }
    /** 정상 종료와 취소의 이벤트 계약을 비교할 Finished 횟수를 기록하는 함수. */
    UFUNCTION() void Finished() { ++FinishedCount; }
    /** 누적 자막 전사기의 동적 오류 전달 횟수를 기록하는 함수. */
    UFUNCTION() void Error(const FString& Message) { ++ErrorCount; }
    TArray<FJWNU_GptLiveTranscript> Transcripts;
    int32 ReadyCount = 0, FinishedCount = 0, ErrorCount = 0;
};
