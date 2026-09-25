# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

"""WebSocket 에코·서버 푸시 및 Unreal 자동 테스트용 ASGI 라우트."""

import asyncio
from contextlib import suppress

from fastapi import APIRouter, Query, WebSocket, WebSocketDisconnect

router = APIRouter()


@router.websocket("/ws/echo")
async def websocket_echo(
    socket: WebSocket,
    mode: str = "echo",
    push_count: int = Query(0, ge=0, le=20),
    delay: float = Query(0.2, ge=0.01, le=5),
):
    """메시지 타입과 본문을 그대로 에코하며 종료할 때까지 연결을 유지한다."""
    if mode == "reject":
        await socket.close(code=1008)
        return
    if mode == "before_open":
        await asyncio.sleep(1)
    offered = socket.scope.get("subprotocols", [])
    protocol = "jwnu.echo" if "jwnu.echo" in offered else None
    await socket.accept(subprotocol=protocol)

    async def push_messages():
        for index in range(push_count):
            await asyncio.sleep(delay)
            await socket.send_json({"Kind": "push", "Index": index, "Text": "안녕 ✈"})

    push_task = asyncio.create_task(push_messages())
    try:
        if mode == "headers":
            await socket.send_json({"Header": socket.headers.get("x-jwnu-test", ""), "Protocol": protocol})
        elif mode == "server_close":
            await asyncio.sleep(delay)
            await socket.close(code=4001, reason="Server requested close")
            return
        elif mode == "drop":
            await asyncio.sleep(delay)
            raise RuntimeError("Intentional JWNU WebSocket disconnect")
        elif mode == "large_binary":
            await socket.send_bytes(bytes(range(256)) * 16)
        elif mode == "large_text":
            await socket.send_text("x" * 4096)

        while True:
            message = await socket.receive()
            if message["type"] == "websocket.disconnect":
                break
            if message.get("text") is not None:
                await socket.send_text(message["text"])
            elif message.get("bytes") is not None:
                await socket.send_bytes(message["bytes"])
    except WebSocketDisconnect:
        pass
    finally:
        push_task.cancel()
        with suppress(asyncio.CancelledError, WebSocketDisconnect, RuntimeError):
            await push_task
