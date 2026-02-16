// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JWNU_BFL_AuthWidget.generated.h"

struct FSlateColor;

UENUM(BlueprintType)
enum class EJWNU_RegisterEmailValidation : uint8
{
	Satisfied,
	Unsatisfied
};

UENUM(BlueprintType)
enum class EJWNU_RegisterFirstPasswordValidation : uint8
{
	Unsatisfied,
	Satisfied,
};

UENUM(BlueprintType)
enum class EJWNU_RegisterSecondPasswordValidation : uint8
{
	FirstPasswordEmpty,
	Unsatisfied,
	Satisfied,
};

UENUM(BlueprintType)
enum class EJWNU_LoginEmailValidation : uint8
{
	Satisfied,
	Unsatisfied
};

UENUM(BlueprintType)
enum class EJWNU_LoginPasswordValidation : uint8
{
	Satisfied,
	Unsatisfied
};

UCLASS(Config=JWNetworkUtility)
class JWNETWORKUTILITYTEST_API UJWNU_BFL_AuthWidget : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	// --- Register ---

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult);

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterCodeTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_RegisterEmailValidation& OutResult);

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterPasswordFirstTextBoxChanged(const FText& Input, EJWNU_RegisterFirstPasswordValidation& OutResult);

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Register")
	static void OnRegisterPasswordSecondTextBoxChanged(const FText& Input, const FText& FirstPassword, EJWNU_RegisterSecondPasswordValidation& OutResult);

	// --- Login ---

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Login")
	static void OnLoginEmailTextBoxChanged(const FText& Input, FText& OutProcessed, EJWNU_LoginEmailValidation& OutResult);

	UFUNCTION(BlueprintCallable, Category = "JWNU|AuthWidget|Login")
	static void OnLoginPasswordTextBoxChanged(const FText& Input, EJWNU_LoginPasswordValidation& OutResult);
	
	// --- Feedback ---
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "JWNU|AuthWidget|Feedback")
	static FSlateColor GetColorBySuccess(const bool bSuccess);

private:
	
	static bool IsPasswordFormatValid(const FString& Password);
};
