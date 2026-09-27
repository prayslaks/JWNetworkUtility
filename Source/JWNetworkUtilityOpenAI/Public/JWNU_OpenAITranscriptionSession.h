// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_OpenAITranscriptionTypes.h"
#include "JWNU_OpenAITranscriptionSession.generated.h"

class UJWNU_WebSocketConnection;
class UJWNU_GIS_OpenAI;
class UWorld;
class FJsonObject;
struct FJWNU_WebSocketError;
struct FJWNU_WebSocketCloseInfo;

/** 장치 독립적인 일회용 Realtime 전사 세션. 모든 공개 호출과 이벤트는 게임 스레드 전용이다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITYOPENAI_API UJWNU_OpenAITranscriptionSession : public UObject
{
    GENERATED_BODY()
public:
    /** 유효한 월드·GameInstance에서 Idle 세션을 만드는 함수. 변수 저장과 이벤트 바인딩 후 Start한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(WorldContext="WorldContextObject", DisplayName="Create OpenAI Transcription Session"))
    static UJWNU_OpenAITranscriptionSession* CreateOpenAITranscriptionSession(const UObject* WorldContextObject);
    /** 이벤트 바인딩 후 호출한다. true는 연결 접수이며 OnReady 이후 오디오를 보낸다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(AutoCreateRefTerm="Options"))
    bool Start(const FJWNU_OpenAITranscriptionOptions& Options, UPARAM(DisplayName="API Key") const FString& ApiKey);
    /** 기본 공식 Endpoint에서만 OPENAI_API_KEY로 연결을 시작하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(AutoCreateRefTerm="Options"))
    bool StartFromEnvironment(const FJWNU_OpenAITranscriptionOptions& Options);
    /** PCM16 LE mono 24kHz. 청크 최대 100ms, 미확정 발화 최대 60초다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") bool AppendInputAudio(const TArray<uint8>& PCM16);
    /** 최소 100ms 입력을 발화로 확정한다. 재사용 세션은 다음 발화 입력을 계속 받는다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") bool CommitInputAudio();
    /** 현재 미확정 오디오를 비운다. 이미 제출한 발화에는 영향이 없다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") bool ClearInputAudio();
    /** 남은 입력을 확정하고 모든 최종 전사까지 기다린다. 100ms 미만 꼬리는 무음 패딩한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") bool Close();
    /** 최종 결과를 기다리지 않고 세션을 즉시 닫는 함수. 활성 세션의 OnClosed는 bFinalized=false다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") void Cancel();
    /** 연결 요청부터 최종 전사 종료 대기까지 실행 중인지 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") bool IsActive() const;
    /** 연결·준비·청취·종료·실패를 구분하는 현재 상태를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") EJWNU_OpenAITranscriptionState GetState() const { return State; }
    /** 입력 PCM16 mono의 고정 표본율 24,000Hz를 반환하는 함수. */
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") int32 GetSampleRate() const { return 24000; }
    /** 확인된 전사 모델 이름. Options.Model 선택 보조용이며 목록 밖 이름도 허용한다. */
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription")
    static TArray<FString> GetKnownTranscriptionModels();

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
    FJWNU_TranscriptionReadyNative OnReadyNative;
    FJWNU_TranscriptNative OnTranscriptNative;
    FJWNU_TranscriptionCommittedNative OnCommittedNative;
    FJWNU_TranscriptionErrorNative OnErrorNative;
    FJWNU_TranscriptionClosedNative OnClosedNative;
    FJWNU_TranscriptionRawNative OnRawEventNative;
    virtual void BeginDestroy() override;
private:
    friend class UJWNU_GIS_OpenAI;
    bool Send(const TSharedRef<FJsonObject>& Event);
    void Connected();
    void Receive(const FString& Json);
    void SocketError(const FJWNU_WebSocketError& Error);
    void SocketClosed(const FJWNU_WebSocketCloseInfo& Info);
    void Fail(const FString& Code, const FString& Message, const FString& EventId = TEXT(""));
    void Finish(const FString& Reason, bool bFinalized);
    void TryFinishClose();
    void ReleaseSocket();
    void Pump();
    /** Realtime 전사 이벤트를 송수신하는 WebSocket 연결 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_WebSocketConnection> Socket;
    TWeakObjectPtr<UJWNU_GIS_OpenAI> OwnerClient;
    TWeakObjectPtr<UWorld> OwnerWorld;
    FJWNU_OpenAITranscriptionOptions Settings;
    EJWNU_OpenAITranscriptionState State = EJWNU_OpenAITranscriptionState::Idle;
    double Deadline = 0;
    int32 BufferedBytes = 0;
    int32 AwaitingCommits = 0;
    TSet<FString> PendingItems;
    bool bHadTranscriptionFailure = false;
};
