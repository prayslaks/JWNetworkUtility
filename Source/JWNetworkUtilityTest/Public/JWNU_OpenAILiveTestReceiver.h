// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_OpenAILiveTypes.h"
#include "JWNU_OpenAILiveTestReceiver.generated.h"

class UJWNU_OpenAILiveSession;
class UJWNU_OpenAILiveComponent;

/** 자동 생성한 BP 그래프의 실제 이벤트 수신을 기록한다. */
UCLASS(Blueprintable)
class UJWNU_OpenAILiveTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    /** Options 미연결 세션 Start를 BP에서 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Live Test") void StartSessionDefaults(UJWNU_OpenAILiveSession* Target);
    /** Options 미연결 환경변수 세션 Start의 BP 컴파일을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Live Test") void StartSessionEnvironmentDefaults(UJWNU_OpenAILiveSession* Target);
    /** Options 미연결 컴포넌트 Start를 BP에서 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Live Test") void StartComponentDefaults(UJWNU_OpenAILiveComponent* Target);
    /** Options 미연결 환경변수 컴포넌트 Start의 BP 컴파일을 검증하는 함수. */
    UFUNCTION(BlueprintImplementableEvent, Category="Live Test") void StartComponentEnvironmentDefaults(UJWNU_OpenAILiveComponent* Target);
    TArray<FJWNU_OpenAILiveTranscript> Transcripts;
    int32 ReadyCount = 0, ClosedCount = 0, ErrorCount = 0, AudioCount = 0;
    FJWNU_OpenAILiveClose LastClose;
    FJWNU_OpenAILiveError LastError;
    TArray<uint8> LastAudio;
    /** 게임 스레드에서 Ready 이벤트가 발생한 횟수를 기록하는 함수. */
    UFUNCTION() void Ready(const FString& Id) { check(IsInGameThread()); ++ReadyCount; }
    /** 게임 스레드에서 종료 횟수와 마지막 종료 정보를 기록하는 함수. */
    UFUNCTION() void Closed(const FJWNU_OpenAILiveClose& Info) { check(IsInGameThread()); ++ClosedCount; LastClose = Info; }
    /** 게임 스레드에서 오류 횟수와 마지막 오류를 기록하는 함수. */
    UFUNCTION() void Error(const FJWNU_OpenAILiveError& Info) { check(IsInGameThread()); ++ErrorCount; LastError = Info; }
    /** 게임 스레드에서 출력 PCM 이벤트 횟수와 마지막 청크를 기록하는 함수. */
    UFUNCTION() void Audio(const TArray<uint8>& Bytes) { check(IsInGameThread()); ++AudioCount; LastAudio = Bytes; }
    /** 자막 이벤트가 생성된 BP 그래프를 통과하는지 검증하는 함수. */
    UFUNCTION(BlueprintNativeEvent, Category="Live Test") void Transcript(const FJWNU_OpenAILiveTranscript& Value);
    virtual void Transcript_Implementation(const FJWNU_OpenAILiveTranscript& Value) { Record(Value); }
    /** 게임 스레드에서 BP가 전달한 자막 이벤트를 순서대로 저장하는 함수. */
    UFUNCTION(BlueprintCallable, Category="Live Test") void Record(const FJWNU_OpenAILiveTranscript& Value) { check(IsInGameThread()); Transcripts.Add(Value); }
};
