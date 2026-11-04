# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
"""이진 전송과 SFX 오프라인 임포트를 검증한다. 외부 서비스나 실제 키를 사용하지 않는다."""
import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import subprocess
import threading
import time

requests = []
errors = []
class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args): pass
    def do_POST(self):
        requests.append(self.path)
        if self.headers.get('X-Test-Key') != 'fake-local-only': errors.append('Missing fixture header')
        expected = b'x' * 70000 if self.path == '/large' else b'{"fixture":true}'
        if self.rfile.read(int(self.headers.get('Content-Length', 0))) != expected: errors.append('Unexpected request body')
        if self.path == '/slow': time.sleep(.5)
        status = 302 if self.path == '/redirect' else 429 if self.path == '/rate' else 200
        self.send_response(status)
        if status == 302: self.send_header('Location', '/must-not-follow')
        self.send_header('X-Fixture', 'ok')
        self.send_header('Content-Length', '4')
        self.end_headers()
        try: self.wfile.write(bytes([0,255,128,1]))
        except (BrokenPipeError, ConnectionResetError): pass

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--engine', type=Path, required=True)
parser.add_argument('--project', type=Path, required=True)
args = parser.parse_args()
output = args.project.resolve().parent/'Saved/Automation/JWAIU-SoundOffline'
output.mkdir(parents=True, exist_ok=True)
with ThreadingHTTPServer(('127.0.0.1',0),Handler) as server:
    thread = threading.Thread(target=server.serve_forever,daemon=True); thread.start()
    try:
        command = [str(args.engine/'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'),str(args.project),'/Engine/Maps/Entry','-unattended','-nop4','-NullRHI','-nosplash','-nosound',
                   f'-JWNUBinaryTestPort={server.server_port}','-ExecCmds=Automation RunTests JWNetworkUtility.BinaryHttp+JWAIUtility.SoundAuthoring','-TestExit=Automation Test Queue Empty',f'-ReportExportPath={output}',f'-abslog={output/"Unreal.log"}']
        with (output/'process.log').open('w',encoding='utf-8') as log:
            result = subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW)
    finally: server.shutdown(); thread.join()
report=json.loads((output/'index.json').read_text(encoding='utf-8-sig'))
passed=report['succeeded']+report.get('succeededWithWarnings',0)
assert result.returncode==0 and passed>=2 and report['failed']==0 and report['notRun']==0, output
assert requests.count('/binary')==2 and requests.count('/redirect')==1 and requests.count('/rate')==1 and requests.count('/slow')==1 and requests.count('/large')==1 and len(requests)==6 and not errors, (requests,errors)
print(f'Passed {passed}; no redirects or retries. Report: {output}')
