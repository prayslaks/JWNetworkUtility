# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

"""Realtime 전사 모의 서버. 키·음성 장치·OpenAI 호출 없이 프로토콜을 검증한다."""
import asyncio
import base64
import os
from fastapi import APIRouter, WebSocket, WebSocketDisconnect

router = APIRouter()

# GA client events 참조(2026-09-26 확인)의 모델별 미지원 필드. 목록 밖 모델은 거절한다.
MODEL_UNSUPPORTED_FIELDS = {
    "gpt-realtime-whisper": ("prompt", "languages", "keywords"),
    "gpt-live-transcribe": ("language",),
    "gpt-transcribe": (),
}


@router.websocket("/realtime")
async def transcription_session(ws: WebSocket, intent: str = "transcription", mode: str = "normal"):
    if intent != "transcription" or (mode != "normal" and os.getenv("JWNU_SSE_TEST_FIXTURES") != "1"):
        await ws.close(code=1008)
        return
    await ws.accept()
    try:
        await ws.send_json({"type": "session.created", "session": {"id": "transcription_mock"}})
        update = await ws.receive_json()
        session = update.get("session", {})
        audio_input = session.get("audio", {}).get("input", {})
        config = audio_input.get("transcription", {})
        model = config.get("model")
        # 모델별 필드 지원 여부는 서버가 판정한다. 클라이언트는 이 규칙을 복제하지 않는다.
        unsupported = MODEL_UNSUPPORTED_FIELDS.get(model)
        valid = (update.get("type") == "session.update" and session.get("type") == "transcription"
                 and audio_input.get("format") == {"type": "audio/pcm", "rate": 24000}
                 and "turn_detection" in audio_input and audio_input["turn_detection"] is None
                 and unsupported is not None and not any(key in config for key in unsupported)
                 and not any(key in session for key in ("model", "instructions", "delegation"))
                 and config.get("delay", "low") in ("minimal", "low", "medium", "high", "xhigh"))
        if not valid or mode == "reject":
            await ws.send_json({"type": "error", "error": {"code": "invalid_session", "message": "Invalid transcription configuration", "event_id": update.get("event_id")}})
            return
        if mode == "start_timeout":
            await asyncio.sleep(3)
            return
        if mode == "bad_format":
            audio_input["format"]["rate"] = 16000
        if mode == "bad_model":
            config["model"] = "wrong-model"
        await ws.send_json({"type": "session.updated", "session": {**session, "id": "transcription_mock"}})
        if mode == "malformed":
            await ws.send_text("{broken")
        if mode == "drop":
            await ws.close(code=1011)
            return
        size, sequence = 0, 1
        previous, pending = None, []

        async def finish(item):
            if mode == "failed_item":
                await ws.send_json({"type": "conversation.item.input_audio_transcription.failed", "item_id": item, "content_index": 0,
                                    "error": {"code": "transcription_failed", "message": "Fixture item failure"}})
            else:
                await ws.send_json({"type": "conversation.item.input_audio_transcription.completed", "item_id": item, "content_index": 0,
                                    "transcript": "안녕 🙂 최종 " + item})

        while True:
            event = await ws.receive_json()
            kind = event.get("type")
            if kind == "input_audio_buffer.append":
                audio = base64.b64decode(event["audio"], validate=True)
                if not audio or len(audio) % 2 or len(audio) > 4800:
                    raise ValueError("Invalid PCM chunk")
                if size == 0:
                    # live 전사는 commit 이전에도 부분 결과가 도착한다.
                    await ws.send_json({"type": "conversation.item.input_audio_transcription.delta", "item_id": f"item_{sequence}",
                                        "content_index": 0, "delta": "안녕 🙂 "})
                size += len(audio)
            elif kind == "input_audio_buffer.clear":
                size = 0
                sequence += 1
                await ws.send_json({"type": "input_audio_buffer.cleared"})
            elif kind == "input_audio_buffer.commit":
                if size < 4800 or mode == "commit_error":
                    await ws.send_json({"type": "error", "error": {"code": "commit_rejected", "message": "Commit rejected", "event_id": event["event_id"]}})
                    continue
                item = f"item_{sequence}"
                sequence += 1
                await ws.send_json({"type": "input_audio_buffer.committed", "item_id": item, "previous_item_id": previous})
                previous, size = item, 0
                if mode == "close_timeout":
                    continue
                if mode == "reverse":
                    pending.append(item)
                    if len(pending) == 2:
                        for value in reversed(pending):
                            await finish(value)
                        pending.clear()
                else:
                    await finish(item)
            else:
                await ws.send_json({"type": "error", "error": {"code": "unsupported_command", "message": "Unexpected event", "event_id": event.get("event_id")}})
    except (WebSocketDisconnect, RuntimeError):
        pass
    except (ValueError, KeyError):
        await ws.close(code=1008)
