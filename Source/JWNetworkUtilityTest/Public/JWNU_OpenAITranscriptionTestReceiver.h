// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_OpenAITranscriptionTypes.h"
#include "JWNU_OpenAITranscriptionTestReceiver.generated.h"
class UJWNU_OpenAITranscriptionSession;
class UJWNU_OpenAITranscriptionComponent;

/** 컴파일한 Blueprint의 전사 이벤트와 Options 미연결 호출을 검증한다. */
UCLASS(Blueprintable)
class UJWNU_OpenAITranscriptionTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintImplementableEvent, Category="Transcription Test") void StartSessionDefaults(UJWNU_OpenAITranscriptionSession* Target);
    UFUNCTION(BlueprintImplementableEvent, Category="Transcription Test") void StartSessionEnvironmentDefaults(UJWNU_OpenAITranscriptionSession* Target);
    UFUNCTION(BlueprintImplementableEvent, Category="Transcription Test") void StartComponentDefaults(UJWNU_OpenAITranscriptionComponent* Target);
    UFUNCTION(BlueprintImplementableEvent, Category="Transcription Test") void StartComponentEnvironmentDefaults(UJWNU_OpenAITranscriptionComponent* Target);
    UFUNCTION(BlueprintNativeEvent, Category="Transcription Test") void Transcript(const FJWNU_OpenAITranscript& Value);
    virtual void Transcript_Implementation(const FJWNU_OpenAITranscript& Value) { Record(Value); }
    UFUNCTION(BlueprintCallable, Category="Transcription Test") void Record(const FJWNU_OpenAITranscript& Value) { check(IsInGameThread()); Transcripts.Add(Value); }
    UFUNCTION() void Ready(const FString& Id) { check(IsInGameThread()); ++ReadyCount; }
    UFUNCTION() void Closed(const FJWNU_OpenAITranscriptionClose& Info) { check(IsInGameThread()); ++ClosedCount; LastClose = Info; }
    UFUNCTION() void Error(const FJWNU_OpenAITranscriptionError& Info) { check(IsInGameThread()); ++ErrorCount; LastError = Info; }
    UFUNCTION() void Committed(const FJWNU_OpenAITranscriptionCommit& Info) { Commits.Add(Info); }
    TArray<FJWNU_OpenAITranscript> Transcripts;
    TArray<FJWNU_OpenAITranscriptionCommit> Commits;
    int32 ReadyCount = 0, ClosedCount = 0, ErrorCount = 0;
    FJWNU_OpenAITranscriptionClose LastClose;
    FJWNU_OpenAITranscriptionError LastError;
};
