// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

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
    UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Live") UJWNU_OpenAILiveSession* GetSession() const { return Session; }

    /** 시작할 때 마이크를 함께 사용할지 결정하는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") bool bUseMicrophone = true;
    /** 시작할 때 출력 PCM을 자동 재생할지 결정하는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") bool bPlayAudio = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="JWNU|OpenAI|Audio") int32 MicrophoneDeviceIndex = -1;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveReadyBP OnReady;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveTranscriptBP OnTranscript;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveAudioBP OnAudio;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveErrorBP OnError;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveClosedBP OnClosed;
    UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Live") FJWNU_LiveUsageBP OnUsage;
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
    UPROPERTY(Transient) TObjectPtr<UJWNU_OpenAILiveSession> Session;
    UPROPERTY(Transient) TObjectPtr<UJWNU_MicrophoneCaptureComponent> Microphone;
    UPROPERTY(Transient) TObjectPtr<UJWNU_PCMPlayerComponent> Player;
    bool bAwaitingClose = false;
    bool bInCallback = false;
    bool bCaptureThisSession = false;
    bool bPlayThisSession = false;
};
