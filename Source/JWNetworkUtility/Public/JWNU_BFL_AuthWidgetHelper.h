// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_BFL_AuthWidgetHelper.generated.h"

class UTextBlock;
struct FSlateColor;

UCLASS(Config=JWNetworkUtility)
class JWNETWORKUTILITY_API UJWNU_BFL_AuthWidgetHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	// --- Register ---

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult);

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterCodeTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult);

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterPrimaryPasswordTextBoxChanged(const FText& Input, EJWNU_RegisterPrimaryPasswordValidation& OutResult);

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterSecondaryPasswordTextBoxChanged(const FText& Input, const FText& FirstPassword, EJWNU_RegisterSecondaryPasswordValidation& OutResult);

	// --- Login ---

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Login")
	static void OnLoginEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_LoginEmailValidation& OutResult);

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Login")
	static void OnLoginPasswordTextBoxChanged(const FText& Input, EJWNU_LoginPasswordValidation& OutResult);
	
	// --- Feedback ---
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "JWNU|AuthWidget|Feedback")
	static FSlateColor GetColorBySuccess(const bool bInSuccess);
	
	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Feedback", meta=(AutoCreateRefTerm="InCover"))
	static void ShowProcessingMessage(UTextBlock* InTextBlock, const FText& InCover);

private:
	
	static bool IsPasswordFormatValid(const FString& InPassword);
};
