// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once
#include "CoreMinimal.h"
#include "JWNU_TypeSafeTypes.generated.h"

/** Jev가 지원하는 판단 종류다. */
UENUM(BlueprintType)
enum class EJWNU_TypeSafeQuestionType : uint8 { Choice, Score, Noul };

/** 일반 문자열과 JSON 문서 입력을 구분한다. */
UENUM(BlueprintType)
enum class EJWNU_TypeSafeStateFormat : uint8 { Text, Json };

/** 호출자가 전송·HTTP·스키마 오류를 구분하는 분류다. */
UENUM(BlueprintType)
enum class EJWNU_TypeSafeErrorCode : uint8 { None, Configuration, Credentials, Network, Timeout, Cancelled, Http, InvalidResponse, ResponseLimit };

/** 문자열은 그대로, JSON은 파싱 후 state 값으로 전송한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeState
{
    GENERATED_BODY()
    /** 입력의 직렬화 형식 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") EJWNU_TypeSafeStateFormat Format = EJWNU_TypeSafeStateFormat::Text;
    /** 텍스트 또는 JSON 문자열·객체·배열 문서 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe", meta=(MultiLine=true)) FString Value;
};

/** Noul의 선택적 true/false 판단 기준이다. 모두 비어 있으면 criteria를 생략한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeNoulCriteria
{
    GENERATED_BODY()
    /** JSON criteria.true에 전달하는 참 판단 설명 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe", meta=(MultiLine=true)) FString TrueDescription;
    /** JSON criteria.false에 전달하는 거짓 판단 설명 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe", meta=(MultiLine=true)) FString FalseDescription;
};

/** 타입별 기준을 가진 질문이다. Json 필드는 고급 구조 입력 시 일반 필드보다 우선한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeQuestion
{
    GENERATED_BODY()
    /** 응답을 찾는 고유 ID이며 모델 지침으로 사용되지 않는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") FString Id;
    /** 판단 종류 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") EJWNU_TypeSafeQuestionType Type = EJWNU_TypeSafeQuestionType::Noul;
    /** 모델에 전달할 완전한 질문 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe", meta=(MultiLine=true)) FString Instructions;
    /** Choice의 선택지 ID와 설명 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") TMap<FString, FString> ChoiceOptions;
    /** Score의 낮은 단계부터 높은 단계까지 설명 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") TArray<FString> ScoreLevels;
    /** Noul의 선택적 Yes 설명 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") FString YesDescription;
    /** Noul의 선택적 No 설명 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") FString NoDescription;
    /** 비어 있지 않으면 Instructions를 대체하는 JSON 문자열·객체·배열 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe", AdvancedDisplay) FString InstructionsJson;
    /** 비어 있지 않으면 타입별 기준을 대체하는 JSON 객체 또는 Score 배열 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe", AdvancedDisplay) FString CriteriaJson;
};

/** 키를 저장하지 않는 요청 설정이다. 재시도 수는 최초 전송을 제외한다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeOptions
{
    GENERATED_BODY()
    /** 전체 평가 URL 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") FString Endpoint = TEXT("https://api.typesafe.ai/v1/systemone");
    /** 모델 이름 또는 고정 버전 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") FString Model = TEXT("jev-latest");
    /** 재시도 대기를 포함한 전체 실시간 제한 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") float TimeoutSeconds = 20;
    /** 한 번의 전송에 허용할 실시간 제한 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") float AttemptTimeoutSeconds = 10;
    /** 최초 전송 이후 최대 재시도 횟수 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe", meta=(ClampMin="0", ClampMax="10")) int32 MaxRetries = 2;
    /** 최초 지수 백오프 지연 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") float InitialRetrySeconds = .25f;
    /** 지수 백오프 상한이며 서버 Retry-After는 이보다 길 수 있는 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") float MaxRetrySeconds = 4;
    /** 수신 본문의 바이트 상한 필드. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TypeSafe") int32 MaxResponseBytes = 2 * 1024 * 1024;
};

/** 선택 결과와 전체 확률 분포다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeChoiceAnswer
{
    GENERATED_BODY()
    /** 선택지 ID 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") FString Choice;
    /** 선택지별 확률 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") TMap<FString, double> Probabilities;
    /** 분포로부터 계산된 확신도이며 정답률과 동일하지 않은 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") double Confidence = 0;
};

/** 순서가 있는 단계의 확률 가중 점수다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeScoreAnswer
{
    GENERATED_BODY()
    /** 단계 사이의 값도 허용하는 점수 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") double Score = 0;
    /** 단계 번호와 설명 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") TMap<FString, FString> Legend;
    /** 단계 번호별 확률 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") TMap<FString, double> Probabilities;
    /** 분포로부터 계산된 확신도 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") double Confidence = 0;
};

/** 별도 confidence 없이 Yes 확률을 반환하는 결과다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeNoulAnswer
{
    GENERATED_BODY()
    /** 0~1의 Yes 확률이며 임계값은 호출자가 결정하는 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") double Noul = 0;
};

/** 질문 ID로 조회하는 타입별 결과와 사용량이다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeResult
{
    GENERATED_BODY()
    /** 실제 응답한 모델 버전 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") FString Model;
    /** Choice 질문 ID별 결과 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") TMap<FString, FJWNU_TypeSafeChoiceAnswer> Choices;
    /** Score 질문 ID별 결과 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") TMap<FString, FJWNU_TypeSafeScoreAnswer> Scores;
    /** Noul 질문 ID별 결과 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") TMap<FString, FJWNU_TypeSafeNoulAnswer> Nouls;
    /** 입력 토큰 사용량 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") int64 InputTokens = 0;
    /** 출력 토큰 사용량 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") int64 OutputTokens = 0;
    /** 최초 전송을 포함한 시도 횟수 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") int32 Attempts = 0;
};

/** HTTP 오류 본문을 보존하며 키·본문을 자동 로그에 기록하지 않는 오류다. */
USTRUCT(BlueprintType)
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeError
{
    GENERATED_BODY()
    /** 오류 분류 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") EJWNU_TypeSafeErrorCode Code = EJWNU_TypeSafeErrorCode::None;
    /** HTTP 응답이 없으면 0인 상태 코드 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") int32 HttpStatus = 0;
    /** 로컬 진단 설명 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") FString Message;
    /** 공급자의 원본 오류 본문 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") FString ResponseBody;
    /** 최초 전송을 포함한 시도 횟수 필드. */
    UPROPERTY(BlueprintReadOnly, Category="TypeSafe") int32 Attempts = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TypeSafeCompletedBP, const FJWNU_TypeSafeResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJWNU_TypeSafeFailedBP, const FJWNU_TypeSafeError&, Error);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TypeSafeCompletedNative, const FJWNU_TypeSafeResult&);
DECLARE_MULTICAST_DELEGATE_OneParam(FJWNU_TypeSafeFailedNative, const FJWNU_TypeSafeError&);
