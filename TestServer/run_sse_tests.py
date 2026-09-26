# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

"""로컬 FastAPI와 Unreal Automation의 SSE 종단 테스트를 실행한다."""

import argparse
import json
import os
from pathlib import Path
import socket
import ssl
import subprocess
import sys
import time
import urllib.request


def main(suite="sse"):
    """HTTP·SSE·WebSocket 공급자 테스트가 서버·UE 프로세스 수명 관리를 공유한다."""
    websocket = suite == "websocket"
    live = suite == "live"
    typesafe = suite == "typesafe"
    api = suite == "api"
    label = "API" if api else ("TypeSafe" if typesafe else ("OpenAI.Live" if live else ("WebSocket" if websocket else "SSE")))
    parser = argparse.ArgumentParser(description=f"Local FastAPI + Unreal {label} integration tests")
    parser.add_argument("--engine", type=Path, required=True, help="UE 설치 루트")
    parser.add_argument("--project", type=Path, required=True, help="호스트 .uproject")
    parser.add_argument("--port", type=int, default=18573)
    if websocket:
        parser.add_argument("--tls-cert", type=Path, help="WSS 시험 서버 인증서 PEM")
        parser.add_argument("--tls-key", type=Path, help="WSS 시험 서버 개인키 PEM")
        parser.add_argument("--expect-untrusted-tls", action="store_true", help="UE에 시험 CA를 주지 않고 거절 여부 검사")
    args = parser.parse_args()
    tls = websocket and args.tls_cert is not None
    if websocket and (bool(args.tls_cert) != bool(args.tls_key) or (args.expect_untrusted_tls and not tls)):
        parser.error("TLS requires --tls-cert and --tls-key")
    tls_args = ["--ssl-certfile", str(args.tls_cert.resolve()), "--ssl-keyfile", str(args.tls_key.resolve())] if tls else []
    health_context = ssl.create_default_context(cafile=str(args.tls_cert.resolve())) if tls else None
    project = args.project.resolve()
    prefix = "JWNUApi-" if api else ("JWNUTypeSafe-" if typesafe else ("JWNUOpenAILive-" if live else ("JWNUWebSocket-" if websocket else "JWNUSse-")))
    output = project.parent / "Saved" / "Automation" / (prefix + time.strftime("%Y%m%d-%H%M%S"))
    output.mkdir(parents=True, exist_ok=True)
    # 이미 실행 중인 다른 서버를 테스트 대상으로 오인하지 않는다.
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", args.port))
    env = os.environ.copy()
    env.update(JWNU_SSE_TEST_FIXTURES="1", PYTHONIOENCODING="utf-8")
    flags = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0
    with (output / "fastapi.log").open("w", encoding="utf-8") as server_log:
        server = subprocess.Popen([sys.executable, "-m", "uvicorn", "main:app",
                                   "--app-dir", str(Path(__file__).resolve().parent),
                                   "--host", "localhost" if tls else "127.0.0.1", "--port", str(args.port)] + tls_args,
                                  env=env, stdout=server_log, stderr=subprocess.STDOUT, creationflags=flags)
        try:
            base = f"{'https' if tls else 'http'}://127.0.0.1:{args.port}"
            socket_base = f"wss://localhost:{args.port}" if tls else f"ws://127.0.0.1:{args.port}"
            for _ in range(100):
                if server.poll() is not None:
                    raise RuntimeError(f"FastAPI failed; see {output / 'fastapi.log'}")
                try:
                    with urllib.request.urlopen(base + "/health", timeout=0.5, context=health_context) as response:
                        if response.status == 200:
                            break
                except OSError:
                    time.sleep(0.1)
            else:
                raise TimeoutError("FastAPI health check timed out")
            if websocket:
                subprocess.run([sys.executable, str(Path(__file__).with_name("websocket_demo.py")),
                                "--url", f"{socket_base}/ws/echo"] + (["--cafile", str(args.tls_cert.resolve())] if tls else []),
                               env=env, creationflags=flags, check=True, timeout=15)
            command = [str(args.engine / "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"), str(project),
                       "/Engine/Maps/Entry",
                       "-unattended", "-nop4", "-NullRHI", "-nosplash", "-nosound",
                       "-JWNUApiIntegration" if api else ("-JWNUTypeSafeIntegration" if typesafe else ("-JWNULiveIntegration" if live else ("-JWNUWebSocketIntegration" if websocket else "-JWNUSseIntegration"))),
                       f"-JWNUApiTestURL={base}" if api else (f"-JWNUTypeSafeTestURL={base}" if typesafe else (f"-JWNULiveTestURL={socket_base}" if live else (f"-JWNUWebSocketTestURL={socket_base}" if websocket else f"-JWNUSseTestURL={base}"))),
                       f"-ExecCmds=Automation RunTests JWNetworkUtility.{label}",
                       "-TestExit=Automation Test Queue Empty", f"-ReportExportPath={output}",
                       f"-abslog={output / 'Unreal.log'}"]
            if tls:
                if args.expect_untrusted_tls:
                    command.append("-JWNUWebSocketExpectTlsFailure")
                else:
                    command.append(f"-ini:Engine:[SSL]:OverrideCertificateBundlePath={args.tls_cert.resolve()}")
            print(f"{label} test report: {output}", flush=True)
            with (output / "console.log").open("w", encoding="utf-8") as unreal_log:
                result = subprocess.run(command, stdout=unreal_log, stderr=subprocess.STDOUT,
                                        creationflags=flags, timeout=900)
            index = output / "index.json"
            if result.returncode or not index.exists():
                raise RuntimeError(f"Unreal tests failed (exit {result.returncode}); see {output}")
            report = json.loads(index.read_text(encoding="utf-8-sig"))
            passed = report.get("succeeded", 0) + report.get("succeededWithWarnings", 0)
            expected = 6 if api else (1 if live else (2 if websocket else 3))
            if report.get("failed", 0) or report.get("notRun", 0) or report.get("inProcess", 0) or passed != expected:
                raise RuntimeError(f"Automation did not pass; see {index}")
            print(f"Passed: {passed}; failed: {report.get('failed', 0)}; with warnings: {report.get('succeededWithWarnings', 0)}", flush=True)
        finally:
            server.terminate()
            try:
                server.wait(timeout=10)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait()


if __name__ == "__main__":
    main()
