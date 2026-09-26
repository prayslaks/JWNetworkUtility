// Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

#include "JWNU_SseTestReceiver.h"
#include "JsonObjectConverter.h"

void UJWNU_SseTestReceiver::ReceiveEvent_Implementation(const FJWNU_SseEvent& Event)
{
	FJWNU_SseTestPayload Payload;
	if (FJsonObjectConverter::JsonObjectStringToUStruct(Event.Data, &Payload, 0, 0)) { RecordParsed(Payload); }
	else { ++ParseErrorCount; }
}

void UJWNU_SseTestReceiver::RecordParsed(const FJWNU_SseTestPayload& Payload)
{
	check(IsInGameThread());
	if (EventCount++ == 0) { FirstEventSeconds = FPlatformTime::Seconds(); }
	LastPayload = Payload;
}

void UJWNU_SseTestReceiver::ReceiveOpened(const FJWNU_SseResponse& Response) { ++OpenCount; LastResponse = Response; }
void UJWNU_SseTestReceiver::ReceiveCompleted(const FJWNU_SseResponse& Response) { ++TerminalCount; LastResponse = Response; CompletedSeconds = FPlatformTime::Seconds(); }
void UJWNU_SseTestReceiver::ReceiveError(const FJWNU_SseResponse& Response) { ++TerminalCount; LastResponse = Response; bCancelled = Response.Error == EJWNU_SseError::Cancelled; }
void UJWNU_SseTestReceiver::ReceiveCancelled() { FJWNU_SseResponse Response; Response.Error = EJWNU_SseError::Cancelled; ReceiveError(Response); }
