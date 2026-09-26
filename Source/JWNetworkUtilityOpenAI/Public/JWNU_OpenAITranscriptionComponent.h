// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JWNU_OpenAITranscriptionTypes.h"
#include "JWNU_OpenAITranscriptionComponent.generated.h"
class UJWNU_OpenAITranscriptionSession;
class UJWNU_MicrophoneCaptureComponent;
struct FJWNU_AudioError;

/** 로컬 마이크를 전사 세션에 연결한다. 자동 시작·복제·서버 VAD는 제공하지 않는다. */
UCLASS(ClassGroup=(JWNU), meta=(BlueprintSpawnableComponent))
class JWNETWORKUTILITYOPENAI_API UJWNU_OpenAITranscriptionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(AutoCreateRefTerm="Options"))
    bool Start(const FJWNU_OpenAITranscriptionOptions& Options, UPARAM(DisplayName="API Key") const FString& ApiKey);
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(AutoCreateRefTerm="Options"))
    bool StartFromEnvironment(const FJWNU_OpenAITranscriptionOptions& Options);
    /** 현재까지 전송한 발화를 확정한다. 마이크 캡처는 계속된다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") bool CommitInputAudio();
    /** 캡처를 멈춘 뒤 마지막 전사 결과까지 기다린다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") void Close();
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") void Cancel();
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") bool IsSessionActive() const;
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") UJWNU_OpenAITranscriptionSession* GetSession() const { return Session; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") bool bUseMicrophone = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") int32 MicrophoneDeviceIndex = -1;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionReadyBP OnReady;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptBP OnTranscript;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionCommittedBP OnCommitted;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionErrorBP OnError;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionClosedBP OnClosed;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionRawBP OnRawEvent;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
private:
    bool Prepare();
    void Ready(const FString& Id);
    void Captured(const TArray<uint8>& Bytes);
    void Error(const FJWNU_OpenAITranscriptionError& Info);
    void AudioError(const FJWNU_AudioError& Info);
    void Closed(const FJWNU_OpenAITranscriptionClose& Info);
    void StopAudio();
    void Cleanup();
    UPROPERTY(Transient) TObjectPtr<UJWNU_OpenAITranscriptionSession> Session;
    UPROPERTY(Transient) TObjectPtr<UJWNU_MicrophoneCaptureComponent> Microphone;
    bool bAwaitingClose = false;
    bool bInCallback = false;
    bool bCaptureThisSession = false;
};
