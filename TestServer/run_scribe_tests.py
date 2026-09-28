# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT

"""키·마이크 없이 loopback Scribe 프로토콜과 Unreal 통합을 검증한다."""
import argparse
import asyncio
import base64
import json
from pathlib import Path
from urllib.parse import parse_qs, urlsplit

from websockets.asyncio.server import serve


async def run(args):
    failures = []
    sessions = []

    async def fixture(socket):
        try:
            query = parse_qs(urlsplit(socket.request.path).query)
            assert query["commit_strategy"] == ["vad"]
            assert query["audio_format"] == ["pcm_16000"]
            assert "xi-api-key" not in socket.request.headers
            await socket.send(json.dumps({"message_type": "session_started", "config": {
                "audio_format": "pcm_16000", "sample_rate": 16000, "model_id": "scribe_v2_realtime"}}))
            chunks = commits = samples = 0
            async for raw in socket:
                message = json.loads(raw)
                assert message["message_type"] == "input_audio_chunk"
                assert message["sample_rate"] == 16000
                pcm = base64.b64decode(message["audio_base_64"], validate=True)
                assert len(pcm) % 2 == 0 and len(pcm) <= 3200
                samples += len(pcm) // 2
                if pcm:
                    chunks += 1
                    if chunks == 1:
                        assert pcm[:4] == b"\x00\x80\xff\x7f"
                        await socket.send(json.dumps({"message_type": "partial_transcript", "text": "대열"}, ensure_ascii=False))
                    if chunks == 2:
                        await socket.send(json.dumps({"message_type": "partial_transcript", "text": "대형 유지"}, ensure_ascii=False))
                    if chunks == 3:
                        await socket.send(json.dumps({"message_type": "committed_transcript", "text": "대형 유지"}, ensure_ascii=False))
                        await socket.send(json.dumps({"message_type": "committed_transcript_with_timestamps", "text": "대형 유지"}, ensure_ascii=False))
                    if chunks == 4:
                        await socket.send(json.dumps({"message_type": "partial_transcript", "text": "대형"}, ensure_ascii=False))
                if message["commit"]:
                    commits += 1
                    assert samples == 32000, "tail or initial short-session padding was lost"
                    assert not pcm and commits == 1, "external/repeated Commit reached server"
                    # 마지막 Final을 지연시켜 첫 확정문만 받고 닫는 구현을 검출한다.
                    await asyncio.sleep(0.3)
                    await socket.send(json.dumps({"message_type": "committed_transcript", "text": "대형 유지"}, ensure_ascii=False))
            assert commits == 1
            sessions.append(samples)
        except Exception as error:
            failures.append(str(error))

    output = args.project.parent / "Saved/Automation/ScribeIntegration"
    output.mkdir(parents=True, exist_ok=True)
    async with serve(fixture, "127.0.0.1", 0) as server:
        port = server.sockets[0].getsockname()[1]
        command = [str(args.engine / "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"), str(args.project),
                   "-unattended", "-nop4", "-nosplash", "-nullrhi", "-nosound",
                   f"-JWNUScribeTestURL=ws://127.0.0.1:{port}/scribe",
                   "-ExecCmds=Automation RunTests JWNetworkUtility.Scribe+ProjectZK.Speech+JWCommonUtility.Security+JWNetworkUtility.OpenAI.Transcription.GptLiveTranscriptor.StreamingResults+JWNetworkUtility.OpenAI.Transcription.GptLiveTranscriptor.Lifetime",
                   "-TestExit=Automation Test Queue Empty", f"-ReportExportPath={output}"]
        with (output / "process.log").open("w", encoding="utf-8") as log:
            process = await asyncio.create_subprocess_exec(*command, stdout=log, stderr=log)
            try:
                result = await asyncio.wait_for(process.wait(), timeout=180)
            except TimeoutError:
                process.kill()
                await process.wait()
                raise
        await asyncio.sleep(0.2)
    report = json.loads((output / "index.json").read_text(encoding="utf-8-sig"))
    print(json.dumps({"exit": result, "passed": report["succeeded"] + report.get("succeededWithWarnings", 0),
                      "warnings": report.get("succeededWithWarnings", 0), "failed": report["failed"],
                      "fixture_sessions": len(sessions), "fixture_errors": failures}, ensure_ascii=False))
    assert result == 0 and report["failed"] == 0 and len(sessions) == 1 and not failures


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--project", type=Path, required=True)
    options = parser.parse_args()
    options.project = options.project.resolve()
    asyncio.run(run(options))
