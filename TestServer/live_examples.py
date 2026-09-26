# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

"""GPT-Live 프로토콜 모의 서버. OpenAI를 호출하지 않으며 녹음을 저장하지 않는다."""
import asyncio
import base64
import os
from fastapi import APIRouter, WebSocket, WebSocketDisconnect

router = APIRouter()


@router.websocket("/live/sessions")
async def live_session(ws: WebSocket, mode: str = "echo"):
    if mode != "echo" and os.getenv("JWNU_SSE_TEST_FIXTURES") != "1":
        await ws.close(code=1008)
        return
    await ws.accept()
    try:
        start = await ws.receive_json()
        session = start.get("session", {})
        if (start.get("type") != "session.start"
                or not session.get("model")
                or session.get("audio", {}).get("format", {}).get("type") != "audio/pcm"
                or session.get("audio", {}).get("format", {}).get("rate") not in (16000, 24000)
                or session.get("delegation", {}).get("type") != "responses"
                or not session.get("delegation", {}).get("responses", {}).get("model")):
            await ws.send_json({"type": "error", "error": {"code": "invalid_start", "message": "Invalid session.start"}})
            return
        if mode == "start_timeout":
            await asyncio.sleep(3)
            return
        if mode == "reject":
            await ws.send_json({"type": "error", "error": {"code": "invalid_model", "message": "Fixture rejected model", "client_event_id": start["event_id"]}})
            return
        if mode == "bad_format":
            session["audio"]["format"]["rate"] = 44100
        await ws.send_json({"type": "session.started", "session": {**session, "id": "live_mock_1"}})
        if mode == "bad_usage":
            await ws.send_json({"type": "session.closed", "usage": {}})
            return
        if mode == "malformed":
            await ws.send_text("{broken")
            await asyncio.sleep(.2)
            return
        if mode == "bad_audio":
            await ws.send_json({"type": "session.output_audio.delta", "delta": base64.b64encode(b"x").decode()})
            return
        if mode == "drop":
            await ws.close(code=1011)
            return
        received = 0
        while True:
            event = await ws.receive_json()
            kind = event.get("type")
            if kind == "session.input_audio.append":
                audio = base64.b64decode(event["audio"], validate=True)
                if not audio or len(audio) % 2:
                    await ws.send_json({"type": "error", "error": {"code": "invalid_audio", "message": "Expected whole PCM16 samples"}})
                    continue
                await ws.send_json({"type": "session.output_audio.delta", "delta": event["audio"]})
                if received == 0:
                    for side, text in (("input", "안녕 🙂 "), ("output", "테스트 응답입니다.")):
                        await ws.send_json({"type": f"session.{side}_transcript.delta", "delta": text, "start_ms": 0, "end_ms": 20})
                    await ws.send_json({"type": "session.usage.updated", "usage": {"seconds": 1}})
                received += 1
            elif kind == "session.close":
                if mode == "close_timeout":
                    await asyncio.sleep(3)
                    return
                await ws.send_json({"type": "session.closed", "reason": "client_request", "usage": {"seconds": 2}, "session": {"id": "live_mock_1"}})
                # 클라이언트가 최종 이벤트 후 소켓을 닫는 순서도 검증한다.
                await ws.receive_text()
                return
            elif kind in ("session.input_audio.mute", "session.input_audio.unmute"):
                await ws.send_json({"type": kind + "d", "client_event_id": event["event_id"]})
            elif kind == "session.instructions.append":
                if event.get("delegation_id", "missing") is not None or not event.get("content"):
                    await ws.send_json({"type": "error", "error": {"code": "invalid_context", "message": "Expected content and null delegation_id"}})
                else:
                    await ws.send_json({"type": "session.instructions.appended", "client_event_id": event["event_id"], "start_ms": 0, "end_ms": 20})
            else:
                await ws.send_json({"type": "error", "error": {"code": "unsupported_command", "message": "Fixture command error", "client_event_id": event.get("event_id")}})
    except (WebSocketDisconnect, RuntimeError):
        pass
