# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.

"""네트워크·마이크 장치 없이 범용 오디오와 UMG 녹음/재생 경로를 검증한다."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--project", required=True, type=Path)
    args = parser.parse_args()
    project = args.project.resolve()
    output = project.parent / "Saved/Automation" / ("JWNUAudio-" + time.strftime("%Y%m%d-%H%M%S"))
    output.mkdir(parents=True, exist_ok=True)
    command = [str(args.engine / "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"), str(project), "/Engine/Maps/Entry",
               "-unattended", "-nop4", "-NullRHI", "-nosplash", "-nosound",
               "-ExecCmds=Automation RunTests JWNetworkUtility.Audio",
               "-TestExit=Automation Test Queue Empty", f"-ReportExportPath={output}", f"-abslog={output / 'Unreal.log'}"]
    print(f"Audio report: {output}", flush=True)
    with (output / "console.log").open("w", encoding="utf-8") as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=900,
                                creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
    index = output / "index.json"
    if result.returncode or not index.exists():
        raise RuntimeError(f"Unreal failed (exit {result.returncode}); see {output}")
    report = json.loads(index.read_text(encoding="utf-8-sig"))
    passed = report.get("succeeded", 0) + report.get("succeededWithWarnings", 0)
    if passed != 2 or any(report.get(key, 0) for key in ("failed", "notRun", "inProcess")):
        raise RuntimeError(f"Audio automation failed; see {index}")
    print(f"Passed: {passed}; failed: 0; with warnings: {report.get('succeededWithWarnings', 0)}", flush=True)


if __name__ == "__main__":
    main()
