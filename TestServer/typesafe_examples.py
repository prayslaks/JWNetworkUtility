# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

"""Jev JSON 계약과 전송 실패를 검증하는 로컬 모의 서버. 실제 모델을 호출하지 않는다."""

import asyncio
import os
from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse, Response

router = APIRouter()
attempts: dict[str, int] = {}


@router.post("/typesafe/v1/systemone")
async def systemone(request: Request, mode: str = "ok", case: str = ""):
    """입력의 질문 ID·기준을 사용해 결정적인 세 종류 응답을 만든다."""
    if mode != "ok" and os.getenv("JWNU_SSE_TEST_FIXTURES") != "1":
        return JSONResponse({"detail": "Test fixtures are disabled"}, status_code=404)
    if request.headers.get("authorization"):
        return JSONResponse({"detail": "Local fixtures must not receive credentials"}, status_code=400)
    if mode in {"401", "422", "429", "529"}:
        return JSONResponse({"detail": "fixture error preserved", "field": "questions"}, status_code=int(mode))
    if mode in {"retry429", "retry529", "retrydate", "retryms"}:
        attempt = attempts.get(case, 0) + 1
        attempts[case] = attempt
        if attempt == 1:
            status = 529 if mode == "retry529" else 429
            headers = {"Retry-After": "1"}
            if mode == "retrydate":
                from datetime import datetime, timedelta, timezone
                from email.utils import format_datetime
                headers = {"Retry-After": format_datetime(datetime.now(timezone.utc) + timedelta(seconds=2), usegmt=True)}
            elif mode == "retryms":
                headers = {"retry-after-ms": "400"}
            return JSONResponse({"detail": "retry fixture"}, status_code=status, headers=headers)
        attempts.pop(case, None)
    if mode == "malformed":
        return Response("not-json", media_type="application/json")
    if mode == "oversize":
        return Response("x" * 65536, media_type="application/json")
    data = await request.json()
    if mode == "delay":
        await asyncio.sleep(1.2)
    if not isinstance(data, dict) or not isinstance(data.get("state"), (str, dict, list)) or not data.get("model") or not data.get("questions"):
        return JSONResponse({"detail": "invalid request shape"}, status_code=422)
    if mode == "utf8" and data["state"] != {"message": "편대 복귀 🙂"}:
        return JSONResponse({"detail": "structured UTF-8 state was not preserved"}, status_code=422)
    answers = {}
    for key, question in data["questions"].items():
        kind = question.get("type")
        if kind == "choice":
            keys = list(question["criteria"])
            probabilities = {option: 0.0 for option in keys}
            probabilities[keys[0]] = 0.75 if len(keys) > 1 else 1.0
            if len(keys) > 1:
                probabilities[keys[1]] = 0.25
            answers[key] = {"type": kind, "choice": keys[0], "probabilities": probabilities, "confidence": 0.6}
        elif kind == "score":
            levels = question["criteria"]
            probabilities = {str(i): 0.0 for i in range(len(levels))}
            probabilities["0"], probabilities["1"] = 0.25, 0.75
            answers[key] = {"type": kind, "score": 0.75, "probabilities": probabilities, "confidence": 0.6,
                            "legend": {str(i): level if isinstance(level, str) else str(level) for i, level in enumerate(levels)}}
        elif kind == "noul":
            answers[key] = {"type": kind, "noul": 0.85}
        else:
            return JSONResponse({"detail": "unknown question type"}, status_code=422)
    if mode == "missing":
        answers.pop(next(iter(answers)))
    if mode == "wrongtype":
        answers[next(iter(answers))]["type"] = "invalid"
    if mode == "badprob":
        for answer in answers.values():
            if answer["type"] == "noul":
                answer["noul"] = 1.5
    return {"model": "fixture-jev", "answers": answers, "usage": {"input_tokens": 42, "output_tokens": 9}}
