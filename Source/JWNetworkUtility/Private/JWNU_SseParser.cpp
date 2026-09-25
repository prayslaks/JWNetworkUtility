// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_SseParser.h"
#include "Containers/StringConv.h"

void FJWNU_SseParser::Reset(const int32 InMaxEventBytes)
{
	*this = FJWNU_SseParser();
	MaxEventBytes = FMath::Max(1, InMaxEventBytes);
}

bool FJWNU_SseParser::Feed(const uint8* Bytes, const int32 Length, TArray<FJWNU_SseEvent>& OutEvents)
{
	for (int32 Index = 0; Index < Length; ++Index)
	{
		const uint8 Byte = Bytes[Index];
		if (bSkipLF)
		{
			bSkipLF = false;
			if (Byte == '\n') { continue; }
		}
		if (Byte == '\r' || Byte == '\n')
		{
			bSkipLF = Byte == '\r';
			if (!ConsumeLine(OutEvents)) { return false; }
		}
		else
		{
			if (Line.Num() >= MaxEventBytes) { return false; }
			Line.Add(Byte);
		}
	}
	return true;
}

bool FJWNU_SseParser::ConsumeLine(TArray<FJWNU_SseEvent>& OutEvents)
{
	// 줄 경계는 UTF-8 문자 내부에 나타나지 않으므로 완성된 줄만 변환한다.
	int32 Offset = 0;
	if (bFirstLine && Line.Num() >= 3 && Line[0] == 0xef && Line[1] == 0xbb && Line[2] == 0xbf) { Offset = 3; }
	bFirstLine = false;
	const int32 LineBytes = Line.Num() - Offset;
	FString Text;
	if (LineBytes > 0)
	{
		const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Line.GetData() + Offset), LineBytes);
		Text = FString(Converted.Length(), Converted.Get());
	}
	Line.Reset();
	if (Text.IsEmpty())
	{
		if (bHasData)
		{
			FJWNU_SseEvent& Event = OutEvents.AddDefaulted_GetRef();
			Event.EventType = EventType.IsEmpty() ? TEXT("message") : EventType;
			Event.Data = MoveTemp(Data);
			Event.Id = LastId;
			Event.RetryMilliseconds = RetryMilliseconds;
		}
		Data.Reset(); EventType.Reset(); EventBytes = 0; bHasData = false;
		return true;
	}
	if (Text.StartsWith(TEXT(":"))) { return true; }
	int32 Colon = INDEX_NONE;
	Text.FindChar(TEXT(':'), Colon);
	const FString Field = Colon == INDEX_NONE ? Text : Text.Left(Colon);
	FString Value = Colon == INDEX_NONE ? FString() : Text.Mid(Colon + 1);
	if (Value.StartsWith(TEXT(" "))) { Value.RightChopInline(1); }
	if (Field == TEXT("data"))
	{
		if (static_cast<int64>(EventBytes) + LineBytes + 1 > MaxEventBytes) { return false; }
		EventBytes += LineBytes + 1;
		if (bHasData) { Data.AppendChar(TEXT('\n')); }
		Data += Value; bHasData = true;
	}
	else if (Field == TEXT("event")) { EventType = MoveTemp(Value); }
	else if (Field == TEXT("id"))
	{
		bool bContainsNull = false;
		for (int32 Index = 0; Index < Value.Len(); ++Index) { bContainsNull |= Value[Index] == 0; }
		if (!bContainsNull) { LastId = MoveTemp(Value); }
	}
	else if (Field == TEXT("retry") && !Value.IsEmpty())
	{
		int64 Parsed = 0;
		for (const TCHAR Character : Value)
		{
			if (Character < '0' || Character > '9' || Parsed > (MAX_int64 - (Character - '0')) / 10) { return true; }
			Parsed = Parsed * 10 + Character - '0';
		}
		RetryMilliseconds = Parsed;
	}
	return true;
}

void FJWNU_SseParser::Finish()
{
	Line.Reset(); Data.Reset(); EventType.Reset(); EventBytes = 0; bHasData = false;
}
