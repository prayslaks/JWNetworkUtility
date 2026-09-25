# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

"""로컬 FastAPI와 Unreal Automation의 SSE 종단 테스트를 실행한다."""

import argparse
import json
import os
from pathlib import Path
import socket
import subprocess
import sys
import time
import urllib.request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True, help="UE 설치 루트")
    parser.add_argument("--project", type=Path, required=True, help="호스트 .uproject")
    parser.add_argument("--port", type=int, default=18573)
    args = parser.parse_args()
    project = args.project.resolve()
    output = project.parent / "Saved" / "Automation" / ("JWNUSse-" + time.strftime("%Y%m%d-%H%M%S"))
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
                                   "--host", "127.0.0.1", "--port", str(args.port)],
                                  env=env, stdout=server_log, stderr=subprocess.STDOUT, creationflags=flags)
        try:
            base = f"http://127.0.0.1:{args.port}"
            for _ in range(100):
                if server.poll() is not None:
                    raise RuntimeError(f"FastAPI failed; see {output / 'fastapi.log'}")
                try:
                    with urllib.request.urlopen(base + "/health", timeout=0.5) as response:
                        if response.status == 200:
                            break
                except OSError:
                    time.sleep(0.1)
            else:
                raise TimeoutError("FastAPI health check timed out")
            command = [str(args.engine / "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"), str(project),
                       "/Engine/Maps/Entry",
                       "-unattended", "-nop4", "-NullRHI", "-nosplash", "-nosound",
                       "-JWNUSseIntegration", f"-JWNUSseTestURL={base}",
                       "-ExecCmds=Automation RunTests JWNetworkUtility.SSE",
                       "-TestExit=Automation Test Queue Empty", f"-ReportExportPath={output}",
                       f"-abslog={output / 'Unreal.log'}"]
            print(f"SSE test report: {output}", flush=True)
            with (output / "console.log").open("w", encoding="utf-8") as unreal_log:
                result = subprocess.run(command, stdout=unreal_log, stderr=subprocess.STDOUT,
                                        creationflags=flags, timeout=900)
            index = output / "index.json"
            if result.returncode or not index.exists():
                raise RuntimeError(f"Unreal tests failed (exit {result.returncode}); see {output}")
            report = json.loads(index.read_text(encoding="utf-8-sig"))
            passed = report.get("succeeded", 0) + report.get("succeededWithWarnings", 0)
            if report.get("failed", 0) or report.get("notRun", 0) or report.get("inProcess", 0) or passed != 2:
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
