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
    /** 명시적 키로 새 전사 세션 연결을 접수하는 함수. 이벤트를 먼저 바인딩하고 OnReady를 기다린다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(AutoCreateRefTerm="Options"))
    bool Start(const FJWNU_OpenAITranscriptionOptions& Options, UPARAM(DisplayName="API Key") const FString& ApiKey);
    /** 기본 공식 Endpoint에서 OPENAI_API_KEY를 읽어 새 세션을 시작하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(AutoCreateRefTerm="Options"))
    bool StartFromEnvironment(const FJWNU_OpenAITranscriptionOptions& Options);
    /** 현재까지 전송한 발화를 확정한다. 마이크 캡처는 계속된다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") bool CommitInputAudio();
    /** 캡처를 멈춘 뒤 마지막 전사 결과까지 기다린다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") void Close();
    /** 마이크와 세션을 즉시 취소하는 함수. 마지막 발화의 최종 결과는 보장하지 않는다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") void Cancel();
    /** 전사 세션이 연결·준비·청취·종료 대기 중인지 반환하는 함수. 컴포넌트 활성 상태와 별개다. */
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") bool IsSessionActive() const;
    /** 가장 최근 전사 세션을 반환하는 함수. 마이크 미사용 시 외부 PCM 입력에 활용한다. */
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") UJWNU_OpenAITranscriptionSession* GetSession() const { return Session; }
    /** 다음 Start에서 로컬 마이크를 함께 사용할지 결정하는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") bool bUseMicrophone = true;
    /** 마이크 캡처 시작에 사용할 장치 인덱스 필드. -1이면 기본 입력 장치다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") int32 MicrophoneDeviceIndex = -1;
    /** 서버가 모델·PCM·수동 발화 설정을 확인해 오디오 입력이 가능함을 알리는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionReadyBP OnReady;
    /** ItemId별 원시 Delta 또는 최종 Transcript를 전달하는 이벤트 필드. 누적·정렬은 호출자 책임이다. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptBP OnTranscript;
    /** 확정된 발화와 이전 발화의 식별자를 전달해 입력 순서를 추적하는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionCommittedBP OnCommitted;
    /** 세션 오류 또는 개별 발화 실패를 알리는 이벤트 필드. bFatal로 전체 중단 여부를 구분한다. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionErrorBP OnError;
    /** 종료 원인과 모든 제출 발화의 성공 여부를 세션당 한 번 전달하는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_TranscriptionClosedBP OnClosed;
    /** 수신 이벤트의 타입과 원문 JSON을 전달하는 이벤트 필드. 원문에 발화 내용이 포함될 수 있다. */
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
    /** 컴포넌트가 생성하여 캡처 PCM과 연결한 원시 전사 세션 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_OpenAITranscriptionSession> Session;
    /** 마이크 사용 시 생성하고 세션 간 재사용하는 캡처 컴포넌트 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_MicrophoneCaptureComponent> Microphone;
    bool bAwaitingClose = false;
    bool bInCallback = false;
    bool bCaptureThisSession = false;
};
