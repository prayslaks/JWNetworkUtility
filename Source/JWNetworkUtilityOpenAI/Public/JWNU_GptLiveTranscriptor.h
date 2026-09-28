// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNU_Transcriptor.h"
#include "JWNU_OpenAITranscriptionTypes.h"
#include "JWNU_GptLiveTranscriptorTypes.h"
#include "JWNU_GptLiveTranscriptor.generated.h"

class UJWNU_GIS_OpenAI;
class UJWNU_OpenAITranscriptionSession;
class UWorld;

/** 외부 mono float PCM을 전사한다. 캡처·VAD·키 저장 없이 청크화와 결과 정렬을 제공한다. 게임 스레드 전용. */
UCLASS(BlueprintType)
class JWNETWORKUTILITYOPENAI_API UJWNU_GptLiveTranscriptor : public UJWNU_Transcriptor
{
	GENERATED_BODY()
public:
	virtual EJWNU_TranscriptBoundaryMode GetBoundaryMode() const override { return EJWNU_TranscriptBoundaryMode::ExternalCommit; }
	/** Create → 변수 저장 → 이벤트 바인딩 → Start. 활성 객체는 OpenAI subsystem이 보관한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(WorldContext="WorldContextObject"))
	static UJWNU_GptLiveTranscriptor* CreateGptLiveTranscriptor(const UObject* WorldContextObject);
	virtual UWorld* GetWorld() const override;
	/** true는 연결 접수다. Ready 이후 PCM을 전달한다. 활성 상태의 재시작은 거절한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(AutoCreateRefTerm="Options"))
	bool Start(const FJWNU_OpenAITranscriptionOptions& Options, UPARAM(DisplayName="API Key") const FString& ApiKey);
	/** 기본 공식 Endpoint에서만 OPENAI_API_KEY를 읽는다. 키 저장 정책은 호출자가 제공한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription", meta=(AutoCreateRefTerm="Options"))
	bool StartFromEnvironment(const FJWNU_OpenAITranscriptionOptions& Options);
	/** 24kHz mono float PCM. 최대 100ms 단위로 나누어 전송하며 미확정 발화는 최대 30초다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") virtual void AppendAudio(const TArray<float>& MonoPCM) override;
	/** 외부 VAD/버튼에서 호출한다. 100ms 미만 발화는 무음 패딩한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") virtual void CommitUtterance() override;
	/** 마지막 발화를 확정하고 모든 최종문을 기다린다. 준비 중이면 취소 후 Finished를 알린다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") virtual void Finish() override;
	/** 즉시 정리한다. 결과·Finished를 발생시키지 않으며 이후 Start로 재사용할 수 있다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|OpenAI|Transcription") virtual void Cancel() override;
	/** 연결 준비부터 최종문 종료 대기까지 활성 실행이 있는지 반환하는 함수. */
	UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") bool IsActive() const { return bActive; }
	/** AppendAudio 입력에 필요한 고정 표본율 24,000Hz를 반환하는 함수. */
	UFUNCTION(BlueprintPure, Category="JWNU|OpenAI|Transcription") virtual int32 GetSampleRate() const override { return 24000; }
	/** 서버 설정 확인을 마쳐 외부 PCM을 받을 수 있음을 알리는 이벤트 필드. */
	UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_GptLiveSignalBP OnReady;
	/** Finish의 종료 처리가 끝났음을 알리는 이벤트 필드. 준비 중 Finish는 취소 후 알린다. */
	UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_GptLiveSignalBP OnFinished;
	/** 발화별 누적 Text를 전달하는 이벤트 필드. 부분문은 즉시, 최종문은 입력 순서대로 전달한다. */
	UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_GptLiveTranscriptBP OnTranscript;
	/** 연결·입력·전사 실패로 현재 실행을 취소한 뒤 원인을 전달하는 이벤트 필드. */
	UPROPERTY(BlueprintAssignable, Category="JWNU|OpenAI|Transcription") FJWNU_GptLiveErrorBP OnError;
	FJWNU_GptLiveSignalNative OnReadyNative;
	FJWNU_GptLiveSignalNative OnFinishedNative;
	FJWNU_GptLiveTranscriptNative OnTranscriptNative;
	FJWNU_GptLiveErrorNative OnErrorNative;
	virtual void BeginDestroy() override;
private:
	friend class UJWNU_GIS_OpenAI;
	friend class FJWNU_GptLiveStreamingTest;
	friend class FJWNU_GptLiveLifetimeTest;
	struct FItem { FString Text; bool bFinal = false; double Deadline = 0; };
	bool StartInternal(const FJWNU_OpenAITranscriptionOptions& Options, const FString& ApiKey, bool bEnvironment);
	void Transcript(const FJWNU_OpenAITranscript& Update);
	void Committed(const FJWNU_OpenAITranscriptionCommit& Commit);
	void FlushFinals();
	void EmitTranscript(const FJWNU_GptLiveTranscript& Result);
	void EmitFinished();
	void Fail(const FString& Message);
	void Poll(double Now);
	static TArray<uint8> EncodePCM(const TArray<float>& Audio, int32 Offset, int32 Count);
	/** 현재 Start 실행의 WebSocket 전사 프로토콜과 원시 이벤트를 담당하는 세션 필드. */
	UPROPERTY(Transient) TObjectPtr<UJWNU_OpenAITranscriptionSession> Session;
	TWeakObjectPtr<UJWNU_GIS_OpenAI> OwnerClient;
	TWeakObjectPtr<UWorld> OwnerWorld;
	TMap<FString, FItem> Items;
	TArray<FString> CommitOrder;
	TArray<double> AwaitingCommits;
	FString LastCommittedId;
	int32 BufferedSamples = 0;
	uint32 Generation = 0;
	bool bActive = false;
	bool bReady = false;
	bool bClosing = false;
};
