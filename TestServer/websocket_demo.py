# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

"""실행 중인 테스트 서버에 텍스트·바이너리를 보내고 정상 종료하는 예제."""

import argparse
import asyncio
import json
import ssl

from websockets.asyncio.client import connect


async def demo(url, cafile=None):
    options = {"ssl": ssl.create_default_context(cafile=cafile)} if url.startswith("wss://") else {}
    async with connect(url, subprotocols=["jwnu.echo"], **options) as socket:
        text = json.dumps({"Text": "안녕 ✈", "Index": 1}, ensure_ascii=False)
        await socket.send(text)
        echoed = await socket.recv()
        assert echoed == text, "Unexpected text echo"
        print("Text:", echoed)
        data = bytes([0, 1, 127, 128, 255])
        await socket.send(data)
        echoed = await socket.recv()
        assert echoed == data, "Unexpected binary echo"
        print("Binary:", list(echoed))
    print("Closed normally")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default="ws://127.0.0.1:5000/ws/echo")
    parser.add_argument("--cafile", help="로컬 TLS 시험 서버의 신뢰할 CA PEM")
    args = parser.parse_args()
    asyncio.run(demo(args.url, args.cafile))
