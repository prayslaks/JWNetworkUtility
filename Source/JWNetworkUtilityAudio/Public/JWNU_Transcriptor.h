// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JWNU_Transcriptor.generated.h"

/** 구현이 고정한 발화 경계 소유권. ProviderManaged에서는 무음도 공급자에게 전달한다. */
UENUM(BlueprintType)
enum class EJWNU_TranscriptBoundaryMode : uint8 { ExternalCommit, ProviderManaged };

/** 공급자 독립적인 전사 구간 갱신. Text 전체로 표시 내용을 교체한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYAUDIO_API FJWNU_TranscriptUpdate
{
	GENERATED_BODY()
	/** 같은 실행 안에서 부분문과 확정문을 연결하는 불투명 구간 식별자. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|Transcription") FString SegmentId;
	/** 현재 구간의 전체 문장. 수정된 부분문도 이 값으로 교체한다. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|Transcription") FString Text;
	/** 원본 append-only 추가분. 누적문 공급자와 최종 결과에서는 비어 있다. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|Transcription") FString Delta;
	/** 구간의 확정 여부. 별도 metadata 이벤트는 다시 Final로 전달하지 않는다. */
	UPROPERTY(BlueprintReadOnly, Category="JWNU|Transcription") bool bFinal = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJWNU_TranscriptorSignalBP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptorUpdateBP, const FJWNU_TranscriptUpdate&, Update);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptorErrorBP, const FString&, Message);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptorUpdateNative, const FJWNU_TranscriptUpdate&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TranscriptorErrorNative, const FString&);

/** 게임 스레드에서 외부 PCM을 처리하는 최소 계약. typed Start와 인증은 각 구현이 소유한다. */
UCLASS(Abstract, BlueprintType)
class JWNETWORKUTILITYAUDIO_API UJWNU_Transcriptor : public UObject
{
	GENERATED_BODY()
public:
	/** 구현이 고정한 경계 정책을 조회한다. 서버 모드에서는 로컬 VAD로 PCM을 제거하지 않는다. */
	UFUNCTION(BlueprintPure, Category="JWNU|Transcription") EJWNU_TranscriptBoundaryMode GetTranscriptBoundaryMode() const { return GetBoundaryMode(); }
	/** 외부 확정을 지원하는 구현에만 Commit을 전달한다. false는 미지원이다. */
	UFUNCTION(BlueprintCallable, Category="JWNU|Transcription") bool RequestCommit() { if (GetBoundaryMode() != EJWNU_TranscriptBoundaryMode::ExternalCommit) return false; CommitUtterance(); return true; }
	virtual EJWNU_TranscriptBoundaryMode GetBoundaryMode() const PURE_VIRTUAL(UJWNU_Transcriptor::GetBoundaryMode, return EJWNU_TranscriptBoundaryMode::ExternalCommit;);
	virtual int32 GetSampleRate() const PURE_VIRTUAL(UJWNU_Transcriptor::GetSampleRate, return 0;);
	virtual void AppendAudio(const TArray<float>& MonoPCM) PURE_VIRTUAL(UJWNU_Transcriptor::AppendAudio, );
	virtual void CommitUtterance() PURE_VIRTUAL(UJWNU_Transcriptor::CommitUtterance, );
	virtual void Finish() PURE_VIRTUAL(UJWNU_Transcriptor::Finish, );
	virtual void Cancel() PURE_VIRTUAL(UJWNU_Transcriptor::Cancel, );
	/** 외부 PCM 입력 준비 완료. */
	UPROPERTY(BlueprintAssignable, Category="JWNU|Transcription") FJWNU_TranscriptorSignalBP OnTranscriptionReady;
	/** 공급자 독립적인 전체 문장과 구간 상태 갱신. */
	UPROPERTY(BlueprintAssignable, Category="JWNU|Transcription") FJWNU_TranscriptorUpdateBP OnTranscriptionUpdate;
	/** 정상 Finish 처리 완료. Cancel은 방송하지 않는다. */
	UPROPERTY(BlueprintAssignable, Category="JWNU|Transcription") FJWNU_TranscriptorSignalBP OnTranscriptionFinished;
	/** 실행을 종료한 오류. 인증 원문은 포함하지 않는다. */
	UPROPERTY(BlueprintAssignable, Category="JWNU|Transcription") FJWNU_TranscriptorErrorBP OnTranscriptionError;
	FSimpleMulticastDelegate OnTranscriptionReadyNative;
	FJWNU_TranscriptorUpdateNative OnTranscriptionUpdateNative;
	FSimpleMulticastDelegate OnTranscriptionFinishedNative;
	FJWNU_TranscriptorErrorNative OnTranscriptionErrorNative;
};
