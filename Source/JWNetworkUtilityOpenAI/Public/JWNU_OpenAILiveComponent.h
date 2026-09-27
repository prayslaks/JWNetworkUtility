// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JWNU_OpenAILiveTypes.h"
#include "JWNU_OpenAILiveComponent.generated.h"
class UJWNU_OpenAILiveSession;
class UJWNU_MicrophoneCaptureComponent;
class UJWNU_PCMPlayerComponent;
struct FJWNU_AudioError;

/** GPT-Live 세션과 로컬 마이크·스피커를 연결하는 BP 편의 컴포넌트다. 자동 시작·복제는 하지 않는다. */
UCLASS(ClassGroup=(JWNU), meta=(BlueprintSpawnableComponent))
class JWNETWORKUTILITYOPENAI_API UJWNU_OpenAILiveComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    /** 직접 전달한 키로 세션을 시작하는 함수. 미연결 Options는 기본값이며 로컬 모의 서버는 빈 키를 사용한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live", meta=(AutoCreateRefTerm="Options"))
    bool Start(const FJWNU_OpenAILiveOptions& Options, UPARAM(DisplayName="API Key") const FString& ApiKey);
    /** OPENAI_API_KEY로 공식 세션을 시작하는 함수. 미연결 Options는 기본값을 사용한다. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live", meta=(AutoCreateRefTerm="Options"))
    bool StartFromEnvironment(const FJWNU_OpenAILiveOptions& Options);
    /** 녹음·재생을 중단하고 세션의 정상 종료를 요청하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live") void Close();
    /** 녹음·재생과 활성 세션을 즉시 취소하는 함수. */
    UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Live") void Cancel();
    /** 컴포넌트가 가장 최근에 생성한 세션을 반환하는 함수. 시작 전에는 null일 수 있다. */
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Live") UJWNU_OpenAILiveSession* GetSession() const { return Session; }

    /** 시작할 때 마이크를 함께 사용할지 결정하는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") bool bUseMicrophone = true;
    /** 시작할 때 출력 PCM을 자동 재생할지 결정하는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") bool bPlayAudio = true;
    /** 캡처 시작 시 사용할 입력 장치 인덱스 필드. -1이면 기본 장치다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") int32 MicrophoneDeviceIndex = -1;
    /** 서버의 session.started 확인을 마쳐 오디오 입력이 가능한 시점을 알리는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveReadyBP OnReady;
    /** 사용자 입력 또는 모델 출력의 자막 추가분과 시간 범위를 전달하는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveTranscriptBP OnTranscript;
    /** 세션 표본율의 PCM16 LE mono 출력 청크를 전달하는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveAudioBP OnAudio;
    /** 공급자·전송 오류를 알리는 이벤트 필드. bFatal로 세션 종료 여부를 구분한다. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveErrorBP OnError;
    /** 세션 종료 정보와 최종 사용량 수신 여부를 전달하는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveClosedBP OnClosed;
    /** 서버가 보고한 사용 시간을 초 단위로 전달하는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveUsageBP OnUsage;
    /** 수신 이벤트의 타입과 원문 JSON을 관찰할 수 있게 전달하는 이벤트 필드. */
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveRawBP OnRawEvent;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
private:
    bool Prepare();
    void Ready(const FString& Id);
    void Audio(const TArray<uint8>& Bytes);
    void Captured(const TArray<uint8>& Bytes);
    void Error(const FJWNU_OpenAILiveError& Info);
    void AudioError(const FJWNU_AudioError& Info);
    void Closed(const FJWNU_OpenAILiveClose& Info);
    void StopAudio();
    /** 이 컴포넌트가 생성하고 오디오 입출력과 연결한 대화 세션 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_OpenAILiveSession> Session;
    /** 마이크 사용 세션에서 생성해 재사용하는 로컬 캡처 컴포넌트 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_MicrophoneCaptureComponent> Microphone;
    /** 서버 출력 PCM을 로컬에서 재생하는 컴포넌트 필드. */
    UPROPERTY(Transient) TObjectPtr<UJWNU_PCMPlayerComponent> Player;
    bool bAwaitingClose = false;
    bool bInCallback = false;
    bool bCaptureThisSession = false;
    bool bPlayThisSession = false;
};
