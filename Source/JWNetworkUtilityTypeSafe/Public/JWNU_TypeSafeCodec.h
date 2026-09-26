// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#pragma once
#include "CoreMinimal.h"
#include "JWNU_TypeSafeTypes.h"

/** UObject 수명이나 네트워크에 의존하지 않는 Jev JSON 계약 검증기다. */
struct JWNETWORKUTILITYTYPESAFE_API FJWNU_TypeSafeCodec
{
    /** 요청을 검증하고 JSON 바디를 만드는 함수. */
    static bool BuildRequest(const FJWNU_TypeSafeState& State, const TArray<FJWNU_TypeSafeQuestion>& Questions, const FString& Model, FString& Body, FString& Error);
    /** 요청의 질문·선택지와 대조해 전체 응답을 원자적으로 파싱하는 함수. */
    static bool ParseResponse(const FString& Body, const TArray<FJWNU_TypeSafeQuestion>& Questions, FJWNU_TypeSafeResult& Result, FString& Error);
};
