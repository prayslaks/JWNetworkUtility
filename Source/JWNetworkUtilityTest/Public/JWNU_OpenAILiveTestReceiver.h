// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_OpenAILiveTypes.h"
#include "JWNU_OpenAILiveTestReceiver.generated.h"

/** 자동 생성한 BP 그래프의 실제 이벤트 수신을 기록한다. */
UCLASS(Blueprintable)
class UJWNU_OpenAILiveTestReceiver : public UObject
{
    GENERATED_BODY()
public:
    TArray<FJWNU_OpenAILiveTranscript> Transcripts;
    int32 ReadyCount = 0, ClosedCount = 0, ErrorCount = 0, AudioCount = 0;
    FJWNU_OpenAILiveClose LastClose;
    FJWNU_OpenAILiveError LastError;
    TArray<uint8> LastAudio;
    UFUNCTION() void Ready(const FString& Id) { check(IsInGameThread()); ++ReadyCount; }
    UFUNCTION() void Closed(const FJWNU_OpenAILiveClose& Info) { check(IsInGameThread()); ++ClosedCount; LastClose = Info; }
    UFUNCTION() void Error(const FJWNU_OpenAILiveError& Info) { check(IsInGameThread()); ++ErrorCount; LastError = Info; }
    UFUNCTION() void Audio(const TArray<uint8>& Bytes) { check(IsInGameThread()); ++AudioCount; LastAudio = Bytes; }
    UFUNCTION(BlueprintNativeEvent, Category="Live Test") void Transcript(const FJWNU_OpenAILiveTranscript& Value);
    virtual void Transcript_Implementation(const FJWNU_OpenAILiveTranscript& Value) { Record(Value); }
    UFUNCTION(BlueprintCallable, Category="Live Test") void Record(const FJWNU_OpenAILiveTranscript& Value) { check(IsInGameThread()); Transcripts.Add(Value); }
};
