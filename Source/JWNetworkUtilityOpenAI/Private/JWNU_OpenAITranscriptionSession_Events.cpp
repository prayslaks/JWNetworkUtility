// Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

#include "JWNU_OpenAITranscriptionSession.h"
#include "JWNU_OpenAITranscriptionJson.h"
#include "UObject/StrongObjectPtr.h"

void UJWNU_OpenAITranscriptionSession::Receive(const FString& Json)
{
    TStrongObjectPtr<UJWNU_OpenAITranscriptionSession> KeepAlive(this);
    if (!IsActive()) { return; }
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) || !Object)
    { Fail(TEXT("protocol"), TEXT("Expected a JSON object.")); return; }
    const FString Type = JWNU::OpenAITranscription::String(Object, TEXT("type"));
    if (Type.IsEmpty()) { Fail(TEXT("protocol"), TEXT("Missing event type.")); return; }
    if (Type == TEXT("session.updated"))
    {
        const TSharedPtr<FJsonObject>* Session = nullptr;
        if (State != EJWNU_OpenAITranscriptionState::Starting || !Object->TryGetObjectField(TEXT("session"), Session)
            || !JWNU::OpenAITranscription::MatchesSession(*Session, Settings))
        { Fail(TEXT("protocol"), TEXT("Resolved transcription session does not match the requested model/PCM/turn detection.")); return; }
        State = EJWNU_OpenAITranscriptionState::Ready;
        const FString Id = JWNU::OpenAITranscription::String(*Session, TEXT("id"));
        OnReadyNative.Broadcast(Id);
        if (State == EJWNU_OpenAITranscriptionState::Ready) { OnReady.Broadcast(Id); }
    }
    else if (Type == TEXT("input_audio_buffer.committed"))
    {
        FJWNU_OpenAITranscriptionCommit Commit;
        Commit.ItemId = JWNU::OpenAITranscription::String(Object, TEXT("item_id"));
        Commit.PreviousItemId = JWNU::OpenAITranscription::String(Object, TEXT("previous_item_id"));
        if (AwaitingCommits <= 0 || Commit.ItemId.IsEmpty() || PendingItems.Contains(Commit.ItemId))
        { Fail(TEXT("protocol"), TEXT("Unexpected or malformed input_audio_buffer.committed.")); return; }
        --AwaitingCommits;
        PendingItems.Add(Commit.ItemId);
        OnCommittedNative.Broadcast(Commit);
        if (IsActive()) { OnCommitted.Broadcast(Commit); }
    }
    else if (Type == TEXT("conversation.item.input_audio_transcription.delta")
        || Type == TEXT("conversation.item.input_audio_transcription.completed"))
    {
        FJWNU_OpenAITranscript Transcript;
        Transcript.ItemId = JWNU::OpenAITranscription::String(Object, TEXT("item_id"));
        Transcript.bFinal = Type.EndsWith(TEXT(".completed"));
        double Index = -1;
        if ((State != EJWNU_OpenAITranscriptionState::Ready && State != EJWNU_OpenAITranscriptionState::Closing)
            || Transcript.ItemId.IsEmpty() || !Object->TryGetNumberField(TEXT("content_index"), Index)
            || !FMath::IsFinite(Index) || Index != 0
            || !Object->TryGetStringField(Transcript.bFinal ? TEXT("transcript") : TEXT("delta"), Transcript.bFinal ? Transcript.Transcript : Transcript.Delta)
            || (Transcript.bFinal && !PendingItems.Contains(Transcript.ItemId)))
        { Fail(TEXT("protocol"), TEXT("Invalid transcript event or unknown completed item.")); return; }
        Transcript.ContentIndex = static_cast<int32>(Index);
        // 콜백에서 Close를 호출해도 최종 자막 BP 전달까지 끝난 뒤 닫히도록 Item을 아직 유지한다.
        OnTranscriptNative.Broadcast(Transcript);
        if (IsActive()) { OnTranscript.Broadcast(Transcript); }
        if (Transcript.bFinal) { PendingItems.Remove(Transcript.ItemId); }
    }
    else if (Type == TEXT("conversation.item.input_audio_transcription.failed"))
    {
        FJWNU_OpenAITranscriptionError Error;
        Error.ItemId = JWNU::OpenAITranscription::String(Object, TEXT("item_id"));
        const TSharedPtr<FJsonObject>* Detail = nullptr;
        if (!PendingItems.Contains(Error.ItemId) || !Object->TryGetObjectField(TEXT("error"), Detail))
        { Fail(TEXT("protocol"), TEXT("Invalid failed transcription item.")); return; }
        Error.Code = JWNU::OpenAITranscription::String(*Detail, TEXT("code"));
        Error.Message = JWNU::OpenAITranscription::String(*Detail, TEXT("message"));
        bHadTranscriptionFailure = true;
        OnErrorNative.Broadcast(Error);
        if (IsActive()) { OnError.Broadcast(Error); }
        PendingItems.Remove(Error.ItemId);
    }
    else if (Type == TEXT("error"))
    {
        const TSharedPtr<FJsonObject>* Detail = nullptr;
        if (!Object->TryGetObjectField(TEXT("error"), Detail))
        { Fail(TEXT("protocol"), TEXT("Missing server error details.")); return; }
        // commit 실패 뒤 로컬/서버 버퍼의 불일치를 피하기 위해 명령 오류는 세션을 종료한다.
        Fail(JWNU::OpenAITranscription::String(*Detail, TEXT("code")), JWNU::OpenAITranscription::String(*Detail, TEXT("message")),
            JWNU::OpenAITranscription::String(*Detail, TEXT("event_id")));
        return;
    }
    if (IsActive()) { OnRawEventNative.Broadcast(Type, Json); }
    if (IsActive()) { OnRawEvent.Broadcast(Type, Json); }
    TryFinishClose();
}
