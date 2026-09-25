// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_OpenAILiveTypes.h"
#include "JWNU_OpenAILiveSession.generated.h"

class UJWNU_WebSocketConnection;
class UJWNU_GIS_OpenAI;
class UWorld;
struct FJWNU_WebSocketError;
struct FJWNU_WebSocketCloseInfo;
class FJsonObject;

/** 장치와 독립적인 일회용 GPT-Live 세션이다. 공개 API와 이벤트는 게임 스레드 전용이다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITYOPENAI_API UJWNU_OpenAILiveSession : public UObject
{
    GENERATED_BODY()
public:
    /** 이벤트를 바인딩한 뒤 Start를 호출할 Idle 세션을 생성하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live", meta=(WorldContext="WorldContextObject"))
    static UJWNU_OpenAILiveSession* CreateLiveSession(const UObject* WorldContextObject);
    /** API 키 또는 직접 제공한 토큰으로 연결을 접수하는 함수. true는 Ready가 아니다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live")
    bool Start(const FJWNU_OpenAILiveOptions& Options, const FString& ApiKey);
    /** 공식 OpenAI 주소에 한해 프로세스 OPENAI_API_KEY를 읽어 연결하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live")
    bool StartFromEnvironment(const FJWNU_OpenAILiveOptions& Options);
    /** Ready 상태에서 PCM16 mono 샘플을 전송하는 함수. 최대 100ms이며 실제 시간에 맞춰 호출한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live")
    bool AppendInputAudio(const TArray<uint8>& PCM16);
    /** 입력 음소거 명령을 전송하는 함수. 수락 여부는 RawEvent의 muted/unmuted로 확인한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live")
    bool SetInputMuted(bool bMuted);
    /** 세션 전체에 신뢰된 지침을 추가하는 함수. Content는 최대 500 토큰이다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live")
    bool AppendInstructions(const FString& Content);
    /** 확장 명령 JSON을 전송하는 함수. 시작·종료·오디오 명령은 전용 함수를 사용한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live")
    bool SendEventJson(const FString& Json);
    /** session.close를 보내고 session.closed 또는 제한 시간까지 기다리는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live")
    bool Close();
    /** 월드 정리·긴급 중단 시 즉시 전송 계층을 닫는 함수. 최종 사용량은 보장하지 않는다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live")
    void Abort();
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Live") EJWNU_OpenAILiveState GetState() const { return State; }
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Live") bool IsActive() const;
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Live") int32 GetSampleRate() const { return Settings.SampleRate; }

    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveReadyBP OnReady;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveTranscriptBP OnTranscript;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveAudioBP OnAudio;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveErrorBP OnError;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveClosedBP OnClosed;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveUsageBP OnUsage;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveRawBP OnRawEvent;
    FJWNU_LiveReadyNative OnReadyNative;
    FJWNU_LiveTranscriptNative OnTranscriptNative;
    FJWNU_LiveAudioNative OnAudioNative;
    FJWNU_LiveErrorNative OnErrorNative;
    FJWNU_LiveClosedNative OnClosedNative;
    FJWNU_LiveUsageNative OnUsageNative;
    FJWNU_LiveRawNative OnRawEventNative;
    virtual void BeginDestroy() override;
private:
    friend class UJWNU_GIS_OpenAI;
    bool Send(const TSharedRef<FJsonObject>& Event);
    void Connected();
    void Receive(const FString& Json);
    void SocketError(const FJWNU_WebSocketError& Error);
    void SocketClosed(const FJWNU_WebSocketCloseInfo& Info);
    void Fail(const FString& Code, const FString& Message, const FString& ClientEventId = TEXT(""));
    void Finish(bool bFailed);
    void ReleaseSocket();
    void Pump();
    UPROPERTY(Transient) TObjectPtr<UJWNU_WebSocketConnection> Socket;
    TWeakObjectPtr<UJWNU_GIS_OpenAI> OwnerClient;
    TWeakObjectPtr<UWorld> OwnerWorld;
    FJWNU_OpenAILiveOptions Settings;
    FJWNU_OpenAILiveClose CloseInfo;
    EJWNU_OpenAILiveState State = EJWNU_OpenAILiveState::Idle;
    double Deadline = 0;
};
