# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

"""생성·바인딩·시작 API 요청의 로컬 수명주기 검증용 응답."""
import asyncio
import os
from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse

router = APIRouter()


@router.api_route("/test/api-request", methods=["GET", "POST"])
async def test_request(request: Request, delay: float = 0, status: int = 200):
    """격리된 로컬 테스트에서만 지연·실패 응답을 제공한다."""
    if os.getenv("JWNU_SSE_TEST_FIXTURES") != "1" or not request.client or request.client.host not in ("127.0.0.1", "::1"):
        return JSONResponse({"detail": "Not found"}, status_code=404)
    await asyncio.sleep(max(0, min(delay, 2)))
    return JSONResponse({"success": 200 <= status < 300, "message": "안녕 API", "query": request.query_params.get("text", ""),
                         "body": (await request.body()).decode("utf-8"),
                         "auth": "fixture-key-ok" if request.headers.get("authorization") == "Bearer fixture-key" else ""},
                        status_code=status if status in (200, 206, 404, 503) else 400)
