// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_KeyCrypto.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#include <dpapi.h>
#endif

bool FJWNU_KeyCrypto::IsProtectionSupported()
{
	return PLATFORM_WINDOWS != 0;
}

bool FJWNU_KeyCrypto::ProtectData(const TArray<uint8>& PlainData, const FString& Context, TArray<uint8>& OutProtectedData)
{
	if (&PlainData == &OutProtectedData) return false;
	ClearSensitiveBytes(OutProtectedData);
	if (PlainData.IsEmpty() || PlainData.Num() > 1024 * 1024 || Context.Len() > 1024) return false;
#if PLATFORM_WINDOWS
	const FTCHARToUTF8 Entropy(*Context);
	DATA_BLOB Input{static_cast<DWORD>(PlainData.Num()), const_cast<BYTE*>(PlainData.GetData())};
	DATA_BLOB Salt{static_cast<DWORD>(Entropy.Length()), reinterpret_cast<BYTE*>(const_cast<ANSICHAR*>(Entropy.Get()))};
	DATA_BLOB Output{};
	// LOCAL_MACHINE을 사용하지 않아 같은 컴퓨터의 다른 계정에 복호화 권한을 주지 않는다.
	if (!CryptProtectData(&Input, nullptr, &Salt, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &Output)) return false;
	OutProtectedData.Append(Output.pbData, Output.cbData);
	LocalFree(Output.pbData);
	return true;
#else
	return false;
#endif
}

bool FJWNU_KeyCrypto::UnprotectData(const TArray<uint8>& ProtectedData, const FString& Context, TArray<uint8>& OutPlainData)
{
	if (&ProtectedData == &OutPlainData) return false;
	ClearSensitiveBytes(OutPlainData);
	if (ProtectedData.IsEmpty() || ProtectedData.Num() > 2 * 1024 * 1024 || Context.Len() > 1024) return false;
#if PLATFORM_WINDOWS
	const FTCHARToUTF8 Entropy(*Context);
	DATA_BLOB Input{static_cast<DWORD>(ProtectedData.Num()), const_cast<BYTE*>(ProtectedData.GetData())};
	DATA_BLOB Salt{static_cast<DWORD>(Entropy.Length()), reinterpret_cast<BYTE*>(const_cast<ANSICHAR*>(Entropy.Get()))};
	DATA_BLOB Output{};
	if (!CryptUnprotectData(&Input, nullptr, &Salt, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &Output)) return false;
	OutPlainData.Append(Output.pbData, Output.cbData);
	SecureZeroMemory(Output.pbData, Output.cbData);
	LocalFree(Output.pbData);
	return true;
#else
	return false;
#endif
}

void FJWNU_KeyCrypto::ClearSensitiveBytes(TArray<uint8>& Data)
{
	volatile uint8* Bytes = Data.GetData();
	for (int32 Index = 0; Index < Data.Num(); ++Index) Bytes[Index] = 0;
	Data.Empty();
}

void FJWNU_KeyCrypto::ClearSensitiveString(FString& Value)
{
	volatile TCHAR* Chars = Value.GetCharArray().GetData();
	for (int32 Index = 0; Index < Value.GetCharArray().Num(); ++Index) Chars[Index] = 0;
	Value.Empty();
}
