// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Engine.h"
#include "JWNU_BFL_AuthWidgetHelper.generated.h"

class UTextBlock;
struct FSlateColor;

/** 인증 UI 입력의 로컬 형식 검증과 처리 중 표시를 제공하는 함수 라이브러리다. */
UCLASS(Config=JWNetworkUtility)
class JWNETWORKUTILITY_API UJWNU_BFL_AuthWidgetHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	// --- Register ---

	/** 이메일 양끝 공백을 제거하고 @ 위치와 도메인의 점 포함 여부를 검사하는 함수. */
	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult);

	/** 숫자만 추출한 입력과 6자리 일치 여부를 반환하는 함수. 서버의 코드 유효성 검증은 별도다. */
	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterCodeTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult);

	/** 10자 이상이며 대문자·소문자·숫자·기타 문자가 포함되는지 검사하는 함수. */
	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterPrimaryPasswordTextBoxChanged(const FText& Input, EJWNU_RegisterPrimaryPasswordValidation& OutResult);

	/** 확인 비밀번호의 일치 여부를 검사하고 첫 비밀번호가 비었으면 별도 결과를 반환하는 함수. */
	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterSecondaryPasswordTextBoxChanged(const FText& Input, const FText& FirstPassword, EJWNU_RegisterSecondaryPasswordValidation& OutResult);

	// --- Login ---

	/** 로그인 이메일 양끝 공백을 제거하고 기본 주소 형식을 검사하는 함수. */
	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Login")
	static void OnLoginEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_LoginEmailValidation& OutResult);

	/** 로그인 비밀번호가 10자 이상인지 검사하는 함수. 실제 인증은 서버가 수행한다. */
	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Login")
	static void OnLoginPasswordTextBoxChanged(const FText& Input, EJWNU_LoginPasswordValidation& OutResult);
	
	// --- Feedback ---
	
	/** 성공이면 초록색, 실패이면 빨간색 피드백 색상을 반환하는 함수. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "JWNU|AuthWidget|Feedback")
	static FSlateColor GetColorBySuccess(const bool bInSuccess);
	
	/** 텍스트 블록을 흰색 처리 중 메시지로 바꾸는 함수. 빈 문구는 기본 안내를 사용한다. */
	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Feedback", meta=(AutoCreateRefTerm="InCover"))
	static void ShowProcessingMessage(UTextBlock* InTextBlock, const FText& InCover);

private:
	
	static bool IsPasswordFormatValid(const FString& InPassword);
};
