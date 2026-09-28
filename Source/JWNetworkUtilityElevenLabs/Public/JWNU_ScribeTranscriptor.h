// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "JWNU_Transcriptor.h"
#include "Tickable.h"
#include "JWNU_ScribeTranscriptor.generated.h"

class UJWNU_WebSocketConnection;

/** Scribe V2 Realtime 설정. 16kHz mono PCM과 서버 VAD는 구현에 고정되어 있다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYELEVENLABS_API FJWNU_ScribeOptions
{
	GENERATED_BODY()
	/** 공식 WSS 주소. 키 없는 loopback WS는 로컬 검증에만 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scribe") FString Endpoint = TEXT("wss://api.elevenlabs.io/v1/speech-to-text/realtime");
	/** ISO 언어 코드. 빈 값은 자동 감지한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scribe") FString Language;
	/** 서버 session_started 응답 대기 시간(초). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scribe", meta=(ClampMin="1")) float StartTimeoutSeconds = 20.f;
	/** 최종 Commit 후 결과를 수신할 시간(초). 종료 ACK가 없어 전체 구간을 기다린다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scribe", meta=(ClampMin="2")) float FinishDrainSeconds = 15.f;
};

/** 캡처·키 저장 없이 Scribe 서버 VAD와 구간별 전체 문장을 제공한다. 호출자가 객체를 보유한다. */
UCLASS(BlueprintType)
class JWNETWORKUTILITYELEVENLABS_API UJWNU_ScribeTranscriptor : public UJWNU_Transcriptor, public FTickableGameObject
{
	GENERATED_BODY()
public:
	/** Create → 변수 보관 → 공통 이벤트 바인딩 → Start 순서로 사용한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|Scribe", meta=(WorldContext="WorldContextObject"))
	static UJWNU_ScribeTranscriptor* CreateScribeTranscriptor(const UObject* WorldContextObject);
	/** API 키는 해당 호출에만 전달하며 직렬화하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|Scribe") bool Start(const FJWNU_ScribeOptions& Options, const FString& ApiKey);
	/** 16kHz mono float PCM을 무음 포함 전달한다. 내부에서 PCM16 LE로 변환한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|Scribe") virtual void AppendAudio(const TArray<float>& MonoPCM) override;
	/** 서버가 경계를 소유하므로 외부 Commit은 수행하지 않는다. Finish만 마지막 확정을 요청한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|Scribe") virtual void CommitUtterance() override;
	/** 잔여 PCM과 마지막 Commit을 전송하고 제한된 결과 수신 구간 이후 종료한다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|Scribe") virtual void Finish() override;
	/** 콜백을 해제하고 즉시 취소한다. 결과와 Finished를 방송하지 않는다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|Scribe") virtual void Cancel() override;
	/** 연결 준비·스트리밍·Finish 결과 수신 중인 실행이 있는지 반환한다. */
	UFUNCTION(BlueprintPure, Category="JWNU|Scribe") bool IsActive() const { return bActive; }
	virtual int32 GetSampleRate() const override { return 16000; }
	virtual EJWNU_TranscriptBoundaryMode GetBoundaryMode() const override { return EJWNU_TranscriptBoundaryMode::ProviderManaged; }
	virtual UWorld* GetWorld() const override;
	virtual void BeginDestroy() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override { return !IsTemplate() && bActive; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UJWNU_ScribeTranscriptor, STATGROUP_Tickables); }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
private:
	friend class FJWNU_ScribeProtocolTest;
	friend class FJWNU_ScribeIntegrationTest;
	void Receive(const FString& Json);
	bool SendChunk(const TArray<float>& Audio, bool bCommit);
	void Fail(const FString& Message);
	void Complete();
	/** 활성 연결을 GC로부터 보유한다. */
	UPROPERTY(Transient) TObjectPtr<UJWNU_WebSocketConnection> Socket;
	TWeakObjectPtr<UWorld> OwnerWorld;
	FJWNU_ScribeOptions Settings;
	TArray<float> PendingAudio;
	FString PartialText;
	uint32 Generation = 0;
	int64 Segment = 1;
	int64 SentSamples = 0;
	double Deadline = 0;
	bool bActive = false;
	bool bReady = false;
	bool bFinishing = false;
};
