// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#pragma once

#include "CoreMinimal.h"
#include "Misc/ScopeLock.h"
#include "Containers/StringConv.h"
#include "JWNU_WebSocketTypes.h"

/** 네트워크 델리게이트가 UObject에 접근하지 않고 데이터를 복사하는 유한 큐다. */
struct FJWNU_WebSocketReceiveQueue
{
	struct FItem
	{
		enum class EKind : uint8 { Connected, Text, Binary, Error, Closed } Kind = EKind::Connected;
		FString Text;
		TArray<uint8> Bytes;
		FJWNU_WebSocketCloseInfo Close;
	};
	explicit FJWNU_WebSocketReceiveQueue(const FJWNU_WebSocketOptions& Options)
		: MaxMessageBytes(Options.MaxMessageBytes), MaxQueuedBytes(Options.MaxQueuedBytes) {}
	FCriticalSection Mutex;
	TArray<FItem> Items;
	TArray<uint8> PartialBinary;
	TArray<uint8> PartialText;
	int64 QueuedBytes = 0;
	int32 MaxMessageBytes;
	int32 MaxQueuedBytes;
	bool bOverflow = false;
	bool bClosed = false;

	bool Reserve(int64 Bytes)
	{
		if (bClosed || bOverflow) { return false; }
		if (Bytes > MaxQueuedBytes - QueuedBytes - PartialBinary.Num() - PartialText.Num())
		{
			bOverflow = true;
			return false;
		}
		QueuedBytes += Bytes;
		return true;
	}
	void Push(FItem Item)
	{
		FScopeLock Lock(&Mutex);
		if (Item.Kind == FItem::EKind::Text)
		{
			const FTCHARToUTF8 Utf8(*Item.Text, Item.Text.Len());
			if (Utf8.Length() > MaxMessageBytes) { bOverflow = true; return; }
		}
		if (Reserve(sizeof(FItem) + (static_cast<int64>(Item.Text.Len()) + Item.Close.Reason.Len()) * sizeof(TCHAR) + Item.Bytes.Num()))
		{
			Items.Add(MoveTemp(Item));
		}
	}
	void PushBinary(const void* Data, SIZE_T Size, bool bLast)
	{
		FScopeLock Lock(&Mutex);
		if (bClosed || bOverflow) { return; }
		if (Size > static_cast<SIZE_T>(MaxMessageBytes - PartialBinary.Num())
			|| Size + sizeof(FItem) > static_cast<uint64>(FMath::Max<int64>(0, MaxQueuedBytes - QueuedBytes - PartialBinary.Num() - PartialText.Num())))
		{
			bOverflow = true; return;
		}
		if (Size > 0) { PartialBinary.Append(static_cast<const uint8*>(Data), static_cast<int32>(Size)); }
		if (bLast)
		{
			QueuedBytes += sizeof(FItem) + PartialBinary.Num();
			auto& Item = Items.AddDefaulted_GetRef();
			Item.Kind = FItem::EKind::Binary;
			Item.Bytes = MoveTemp(PartialBinary);
		}
	}
	void PushTextFragment(const void* Data, SIZE_T Size, bool bLast)
	{
		FScopeLock Lock(&Mutex);
		if (bClosed || bOverflow) { return; }
		if (Size > static_cast<SIZE_T>(MaxMessageBytes - PartialText.Num())
			|| Size + sizeof(FItem) > static_cast<uint64>(FMath::Max<int64>(0, MaxQueuedBytes - QueuedBytes - PartialText.Num() - PartialBinary.Num())))
		{
			bOverflow = true; return;
		}
		if (Size) { PartialText.Append(static_cast<const uint8*>(Data), static_cast<int32>(Size)); }
		if (bLast)
		{
			// 수신 청크의 경계는 UTF-8 문자의 경계가 아니다. 메시지 전체에서 한 번만 변환한다.
			const FUTF8ToTCHAR Utf8(PartialText.IsEmpty() ? "" : reinterpret_cast<const ANSICHAR*>(PartialText.GetData()), PartialText.Num());
			const int64 Cost = sizeof(FItem) + static_cast<int64>(Utf8.Length()) * sizeof(TCHAR);
			PartialText.Reset();
			if (!Reserve(Cost)) { return; }
			auto& Item = Items.AddDefaulted_GetRef();
			Item.Kind = FItem::EKind::Text;
			Item.Text = FString(Utf8.Length(), Utf8.Get());
		}
	}
	bool Take(TArray<FItem>& Out)
	{
		FScopeLock Lock(&Mutex);
		Out = MoveTemp(Items);
		QueuedBytes = 0;
		return !bOverflow;
	}
	void Stop()
	{
		FScopeLock Lock(&Mutex);
		bClosed = true;
		Items.Reset(); PartialBinary.Reset(); PartialText.Reset(); QueuedBytes = 0;
	}
};
