"""
JWNetworkUtility Test Server
=============================
플러그인의 API 클라이언트 기능을 테스트하기 위한 FastAPI 서버.

실행:
    pip install -r requirements.txt
    python main.py

또는:
    uvicorn main:app --host 0.0.0.0 --port 5000 --reload

SMTP 설정 (.env 파일 또는 환경변수):
    SMTP_HOST=smtp.gmail.com
    SMTP_PORT=587
    SMTP_USER=your-email@gmail.com
    SMTP_PASSWORD=your-app-password
    SMTP_FROM=your-email@gmail.com  (선택, 기본값=SMTP_USER)

테스트 시나리오:
    1. GET  /health                      - 헬스체크 (인증 불필요)
    2. POST /auth/register/send-code     - 이메일 인증코드 발송
    3. POST /auth/register/verify-code   - 인증코드 검증
    4. POST /auth/register               - 회원가입 완료
    5. POST /auth/login                  - 로그인 (JWT 발급)
    6. POST /auth/refresh                - 토큰 리프레시
    7. POST /auth/reset                  - 전체 데이터 초기화
    8. GET/POST/PUT/DELETE /api/data     - 인증 필요 엔드포인트
"""

import asyncio
import hashlib
import os
import random
import smtplib
import string
import time
import uuid
import argparse
from contextlib import asynccontextmanager
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from typing import Optional

import jwt
from dotenv import load_dotenv
from fastapi import FastAPI, Header, Query, Request
from fastapi.responses import JSONResponse
from pydantic import BaseModel

# .env 파일 로드
load_dotenv()

# ──────────────────────────────────────────────
# 로그 다국어 (LOG_LANG: ko / en)
# ──────────────────────────────────────────────

LOG_LANG = os.getenv("LOG_LANG", "ko").lower()

_LOG_MESSAGES: dict[str, dict[str, str]] = {
    "smtp_status_not_configured": {
        "ko": "미설정 (콘솔 출력 모드)",
        "en": "Not configured (console output mode)",
    },
    "smtp_not_configured": {
        "ko": "[SMTP 미설정] 콘솔에서 인증코드를 확인하세요.",
        "en": "[SMTP NOT CONFIGURED] Check the verification code in the console.",
    },
    "smtp_email_label": {
        "ko": "이메일: {email}",
        "en": "Email: {email}",
    },
    "smtp_code_label": {
        "ko": "인증코드: {code}",
        "en": "Verification code: {code}",
    },
    "smtp_sent": {
        "ko": "[SMTP] 인증코드 발송 완료 → {email}",
        "en": "[SMTP] Verification code sent → {email}",
    },
    "smtp_error": {
        "ko": "[SMTP 에러] {error}",
        "en": "[SMTP ERROR] {error}",
    },
    "register_complete": {
        "ko": "[회원가입] {email} 가입 완료 (총 유저 수: {count})",
        "en": "[REGISTER] {email} registered (total users: {count})",
    },
}


def log(key: str, **kwargs: object) -> str:
    """LOG_LANG에 따라 로그 메시지를 반환한다."""
    msg = _LOG_MESSAGES[key]
    template = msg.get(LOG_LANG, msg["en"])
    return template.format(**kwargs) if kwargs else template


# ──────────────────────────────────────────────
# 설정
# ──────────────────────────────────────────────

SECRET_KEY = "jwnetworkutility-test-secret-key"

# 짧게 설정하여 리프레시 플로우 테스트 용이
ACCESS_TOKEN_EXPIRE_SECONDS = 15
REFRESH_TOKEN_EXPIRE_SECONDS = 60

# 인증코드 5분 유효
VERIFICATION_CODE_EXPIRE_SECONDS = 300  

# SMTP 설정
SMTP_HOST = os.getenv("SMTP_HOST", "")
SMTP_PORT = int(os.getenv("SMTP_PORT", "587"))
SMTP_USER = os.getenv("SMTP_USER", "")
SMTP_PASSWORD = os.getenv("SMTP_PASSWORD", "")
SMTP_FROM = os.getenv("SMTP_FROM", "") or SMTP_USER

# ──────────────────────────────────────────────
# 인메모리 저장소 (서버 종료 시 소멸)
# ──────────────────────────────────────────────

# 유저 저장소: { email: { "password_hash": str, "created_at": float } }
user_store: dict[str, dict] = {}

# 인증코드 저장소: { email: { "code": str, "created_at": float, "verified": bool } }
verification_store: dict[str, dict] = {}

# 리프레시 토큰 저장소: { token_string: { "user_id": str, "target_server": str, ... } }
refresh_token_store: dict[str, dict] = {}

@asynccontextmanager
async def lifespan(app: FastAPI):
    smtp_status = f"{SMTP_HOST}:{SMTP_PORT}" if SMTP_HOST else log("smtp_status_not_configured")
    print(f"")
    print(f"  ╔══════════════════════════════════════════╗")
    print(f"  ║   JWNetworkUtility Test Server           ║")
    print(f"  ╠══════════════════════════════════════════╣")
    print(f"  ║  SMTP: {smtp_status:<33}║")
    print(f"  ║  Access Token TTL:  {ACCESS_TOKEN_EXPIRE_SECONDS:>5}s              ║")
    print(f"  ║  Refresh Token TTL: {REFRESH_TOKEN_EXPIRE_SECONDS:>5}s              ║")
    print(f"  ║  Verify Code TTL:   {VERIFICATION_CODE_EXPIRE_SECONDS:>5}s              ║")
    print(f"  ╚══════════════════════════════════════════╝")
    print(f"")
    yield

app = FastAPI(title="JWNetworkUtility Test Server", lifespan=lifespan)


# ──────────────────────────────────────────────
# 응답 모델 (UPROPERTY 이름과 일치)
# ──────────────────────────────────────────────

class BaseResponse(BaseModel):
    """FJWNU_BaseResponseData 대응"""
    Success: bool
    Code: str
    Message: str


class AuthResponse(BaseResponse):
    """FJWNU_RefreshTokenResponseData 대응"""
    AccessToken: str = ""
    ExpiresAt: int = 0
    RefreshToken: str = ""
    RefreshTokenExpiresAt: int = 0
    UserId: str = ""


class DataResponse(BaseResponse):
    """테스트용 데이터 응답"""
    Data: str = ""


# ──────────────────────────────────────────────
# 요청 모델
# ──────────────────────────────────────────────

class SendCodeRequest(BaseModel):
    email: str


class VerifyCodeRequest(BaseModel):
    email: str
    code: str


class RegisterRequest(BaseModel):
    email: str
    password: str
    code: str


class LoginRequest(BaseModel):
    email: str
    password: str


class RefreshRequest(BaseModel):
    """플러그인의 ExecuteTokenRefresh가 보내는 형식"""
    userId: str = ""
    targetServer: str = ""
    refreshToken: str = ""


class DataRequest(BaseModel):
    data: str = ""


# ──────────────────────────────────────────────
# 헬퍼 함수
# ──────────────────────────────────────────────

def hash_password(password: str) -> str:
    """SHA-256 비밀번호 해시 (테스트 서버용)."""
    return hashlib.sha256(password.encode("utf-8")).hexdigest()


def generate_verification_code() -> str:
    """6자리 숫자 인증코드 생성."""
    return "".join(random.choices(string.digits, k=6))


def send_verification_email(to_email: str, code: str) -> bool:
    """SMTP로 인증코드 이메일 발송. SMTP 미설정 시 콘솔 출력."""
    if not SMTP_HOST or not SMTP_USER:
        print(f"")
        print(f"  {log('smtp_not_configured')}")
        print(f"  {log('smtp_email_label', email=to_email)}")
        print(f"  {log('smtp_code_label', code=code)}")
        print(f"")
        return True

    try:
        msg = MIMEMultipart()
        msg["From"] = SMTP_FROM
        msg["To"] = to_email
        msg["Subject"] = "[JWNetworkUtility] 이메일 인증 코드"

        body = (
            f"안녕하세요,\n\n"
            f"요청하신 이메일 인증 코드입니다:\n\n"
            f"    {code}\n\n"
            f"이 코드는 {VERIFICATION_CODE_EXPIRE_SECONDS // 60}분간 유효합니다.\n\n"
            f"본인이 요청하지 않았다면 이 이메일을 무시하세요."
        )
        msg.attach(MIMEText(body, "plain", "utf-8"))

        if SMTP_PORT == 465:
            server = smtplib.SMTP_SSL(SMTP_HOST, SMTP_PORT)     
        else:
            server = smtplib.SMTP(SMTP_HOST, SMTP_PORT)
            server.starttls()

        server.login(SMTP_USER, SMTP_PASSWORD)
        server.send_message(msg)
        server.quit()
        print(f"  {log('smtp_sent', email=to_email)}")
        return True
    except Exception as e:
        print(f"  {log('smtp_error', error=e)}")
        return False


def create_access_token(user_id: str) -> tuple[str, int]:
    """JWT 엑세스 토큰 생성. (토큰 문자열, 만료 유닉스 타임스탬프) 반환."""
    expires_at = int(time.time()) + ACCESS_TOKEN_EXPIRE_SECONDS
    payload = {
        "sub": user_id,
        "exp": expires_at,
        "type": "access",
    }
    token = jwt.encode(payload, SECRET_KEY, algorithm="HS256")
    return token, expires_at


def create_refresh_token(user_id: str, target_server: str) -> tuple[str, int]:
    """리프레시 토큰 생성 및 인메모리 저장. (토큰 문자열, 만료 유닉스 타임스탬프) 반환."""
    token_id = str(uuid.uuid4())
    expires_at = int(time.time()) + REFRESH_TOKEN_EXPIRE_SECONDS
    refresh_token_store[token_id] = {
        "user_id": user_id,
        "target_server": target_server,
        "created_at": time.time(),
        "expires_at": expires_at,
    }
    return token_id, expires_at


def verify_access_token(authorization: Optional[str]) -> dict | None:
    """
    Authorization 헤더에서 Bearer 토큰을 추출하고 검증.
    성공 시 payload dict, 실패 시 None 반환.
    """
    if not authorization:
        return None

    token = authorization.removeprefix("Bearer ").strip()
    if not token:
        return None

    try:
        payload = jwt.decode(token, SECRET_KEY, algorithms=["HS256"])
        return payload
    except (jwt.ExpiredSignatureError, jwt.InvalidTokenError):
        return None


# ──────────────────────────────────────────────
# 인증 미들웨어 (보호된 /api/** 경로만 적용)
# ──────────────────────────────────────────────

@app.middleware("http")
async def auth_middleware(request: Request, call_next):
    """
    /api/ 로 시작하는 경로에만 Bearer 토큰 검증 적용.
    실패 시 HTTP 401 반환 → 플러그인의 401 핸들러 / 잡 큐가 동작.
    """
    if request.url.path.startswith("/api/"):
        auth_header = request.headers.get("Authorization")
        payload = verify_access_token(auth_header)
        if payload is None:
            return JSONResponse(
                status_code=401,
                content={"Success": False, "Code": "UNAUTHORIZED", "Message": "Unauthorized"},
            )
        request.state.user = payload

    return await call_next(request)


# ──────────────────────────────────────────────
# 엔드포인트: 헬스체크 (인증 불필요)
# ──────────────────────────────────────────────

@app.get("/health")
def health_check():
    """bRequiresAuth=false 테스트용."""
    return BaseResponse(
        Success=True,
        Code="HEALTH_OK",
        Message="Server is running",
    )


# ──────────────────────────────────────────────
# 엔드포인트: 회원가입
# ──────────────────────────────────────────────

@app.post("/auth/register/send-code")
def register_send_code(req: SendCodeRequest):
    """1단계: 이메일 중복 확인 후 인증코드 발송."""
    email = req.email.strip().lower()

    if not email or "@" not in email:
        return BaseResponse(
            Success=False,
            Code="INVALID_EMAIL",
            Message="유효한 이메일 주소를 입력하세요.",
        )

    # 이메일 중복 확인
    if email in user_store:
        return BaseResponse(
            Success=False,
            Code="EMAIL_ALREADY_EXISTS",
            Message="이미 가입된 이메일입니다.",
        )

    # 인증코드 재발송 쿨다운 (60초)
    existing = verification_store.get(email)
    if existing and (time.time() - existing["created_at"]) < 60:
        remaining = int(60 - (time.time() - existing["created_at"]))
        return BaseResponse(
            Success=False,
            Code="CODE_COOLDOWN",
            Message=f"인증코드를 이미 발송했습니다. {remaining}초 후에 다시 시도하세요.",
        )

    # 인증코드 생성 및 저장
    code = generate_verification_code()
    verification_store[email] = {
        "code": code,
        "created_at": time.time(),
        "verified": False,
    }

    # 이메일 발송
    if not send_verification_email(email, code):
        return BaseResponse(
            Success=False,
            Code="EMAIL_SEND_FAILED",
            Message="인증코드 이메일 발송에 실패했습니다. 잠시 후 다시 시도하세요.",
        )

    return BaseResponse(
        Success=True,
        Code="CODE_SENT",
        Message="인증코드가 이메일로 발송되었습니다.",
    )


@app.post("/auth/register/verify-code")
def register_verify_code(req: VerifyCodeRequest):
    """2단계: 인증코드 검증."""
    email = req.email.strip().lower()
    code = req.code.strip()

    stored = verification_store.get(email)
    if stored is None:
        return BaseResponse(
            Success=False,
            Code="CODE_NOT_FOUND",
            Message="인증코드를 먼저 요청하세요.",
        )

    # 만료 확인
    if (time.time() - stored["created_at"]) > VERIFICATION_CODE_EXPIRE_SECONDS:
        del verification_store[email]
        return BaseResponse(
            Success=False,
            Code="CODE_EXPIRED",
            Message="인증코드가 만료되었습니다. 다시 요청하세요.",
        )

    # 코드 일치 확인
    if stored["code"] != code:
        return BaseResponse(
            Success=False,
            Code="CODE_MISMATCH",
            Message="인증코드가 일치하지 않습니다.",
        )

    # 검증 완료 마킹
    stored["verified"] = True
    return BaseResponse(
        Success=True,
        Code="CODE_VERIFIED",
        Message="이메일 인증이 완료되었습니다.",
    )


@app.post("/auth/register")
def register(req: RegisterRequest):
    """3단계: 회원가입 완료. 인증코드 검증 상태를 확인하고 계정 생성."""
    email = req.email.strip().lower()
    password = req.password
    code = req.code.strip()

    if not email or "@" not in email:
        return BaseResponse(
            Success=False,
            Code="INVALID_EMAIL",
            Message="유효한 이메일 주소를 입력하세요.",
        )

    if not password or len(password) < 4:
        return BaseResponse(
            Success=False,
            Code="INVALID_PASSWORD",
            Message="비밀번호는 4자 이상이어야 합니다.",
        )

    # 이메일 중복 재확인
    if email in user_store:
        return BaseResponse(
            Success=False,
            Code="EMAIL_ALREADY_EXISTS",
            Message="이미 가입된 이메일입니다.",
        )

    # 인증코드 검증 상태 확인
    stored = verification_store.get(email)
    if stored is None:
        return BaseResponse(
            Success=False,
            Code="CODE_NOT_FOUND",
            Message="인증코드를 먼저 요청하세요.",
        )

    if not stored["verified"]:
        return BaseResponse(
            Success=False,
            Code="CODE_NOT_VERIFIED",
            Message="이메일 인증을 먼저 완료하세요.",
        )

    if stored["code"] != code:
        return BaseResponse(
            Success=False,
            Code="CODE_MISMATCH",
            Message="인증코드가 일치하지 않습니다.",
        )

    # 만료 확인
    if (time.time() - stored["created_at"]) > VERIFICATION_CODE_EXPIRE_SECONDS:
        del verification_store[email]
        return BaseResponse(
            Success=False,
            Code="CODE_EXPIRED",
            Message="인증코드가 만료되었습니다. 다시 요청하세요.",
        )

    # 유저 생성
    user_store[email] = {
        "password_hash": hash_password(password),
        "created_at": time.time(),
    }

    # 인증코드 정리
    del verification_store[email]

    print(f"  {log('register_complete', email=email, count=len(user_store))}")

    return BaseResponse(
        Success=True,
        Code="REGISTER_SUCCESS",
        Message="회원가입이 완료되었습니다.",
    )


# ──────────────────────────────────────────────
# 엔드포인트: 로그인 / 토큰
# ──────────────────────────────────────────────

@app.post("/auth/login")
def login(req: LoginRequest):
    """이메일 + 비밀번호로 로그인. JWT 토큰 발급."""
    email = req.email.strip().lower()

    if not email or not req.password:
        return AuthResponse(
            Success=False,
            Code="INVALID_CREDENTIALS",
            Message="이메일과 비밀번호를 입력하세요.",
        )

    # 유저 존재 확인
    user = user_store.get(email)
    if user is None:
        return AuthResponse(
            Success=False,
            Code="USER_NOT_FOUND",
            Message="등록되지 않은 이메일입니다.",
        )

    # 비밀번호 확인
    if user["password_hash"] != hash_password(req.password):
        return AuthResponse(
            Success=False,
            Code="WRONG_PASSWORD",
            Message="비밀번호가 일치하지 않습니다.",
        )

    access_token, expires_at = create_access_token(email)
    refresh_token, refresh_expires_at = create_refresh_token(email, "GameServer")

    return AuthResponse(
        Success=True,
        Code="LOGIN_SUCCESS",
        Message=f"{email} 로그인 성공",
        AccessToken=access_token,
        ExpiresAt=expires_at,
        RefreshToken=refresh_token,
        RefreshTokenExpiresAt=refresh_expires_at,
        UserId=email,
    )


@app.post("/auth/refresh")
def refresh(req: RefreshRequest):
    """
    플러그인의 ExecuteTokenRefresh가 호출하는 엔드포인트.
    요청 형식: { "userId": "...", "targetServer": "...", "refreshToken": "..." }
    """
    stored = refresh_token_store.pop(req.refreshToken, None)
    if stored is None:
        return AuthResponse(
            Success=False,
            Code="INVALID_REFRESH_TOKEN",
            Message="Refresh token not found or already used",
        )

    # 만료 확인
    if time.time() > stored["expires_at"]:
        return AuthResponse(
            Success=False,
            Code="REFRESH_TOKEN_EXPIRED",
            Message="Refresh token has expired",
        )

    user_id = stored["user_id"]
    access_token, expires_at = create_access_token(user_id)
    new_refresh_token, refresh_expires_at = create_refresh_token(user_id, req.targetServer)

    return AuthResponse(
        Success=True,
        Code="REFRESH_SUCCESS",
        Message="Token refreshed successfully",
        AccessToken=access_token,
        ExpiresAt=expires_at,
        RefreshToken=new_refresh_token,
        RefreshTokenExpiresAt=refresh_expires_at,
        UserId=user_id,
    )


# ──────────────────────────────────────────────
# 엔드포인트: 보호된 API (인증 필요)
# ──────────────────────────────────────────────
# 네트워크 딜레이 시뮬레이션 쿼리 파라미터:
#   ?delay=<seconds>   — 응답 전 대기 (최대 60초)
#   ?status=<code>     — 딜레이 후 지정 HTTP 상태코드로 응답 (5xx 재시도 테스트)
#
# 예시:
#   GET  /api/data?delay=5          → 5초 후 정상 응답
#   GET  /api/data?delay=30         → 30초 딜레이 (타임아웃 테스트)
#   GET  /api/data?delay=2&status=500 → 2초 후 500 에러 (재시도 테스트)
#   POST /api/data?delay=1&status=503 → 1초 후 503 에러

MAX_SIMULATED_DELAY = 60  # 최대 딜레이 (초)


async def _apply_delay_and_status(
    delay: float | None,
    status: int | None,
) -> JSONResponse | None:
    """딜레이 적용 후, status가 지정됐으면 에러 응답을 반환. 정상이면 None."""
    if delay is not None and delay > 0:
        clamped = min(delay, MAX_SIMULATED_DELAY)
        print(f"  [DELAY] Sleeping {clamped:.1f}s ...")
        await asyncio.sleep(clamped)

    if status is not None and status >= 400:
        return JSONResponse(
            status_code=status,
            content={
                "Success": False,
                "Code": f"SIMULATED_{status}",
                "Message": f"Simulated HTTP {status} error",
            },
        )
    return None


@app.get("/api/data")
async def get_data(
    request: Request,
    delay: float | None = Query(None, description="응답 딜레이 (초)"),
    status: int | None = Query(None, description="시뮬레이션할 HTTP 상태코드"),
):
    err = await _apply_delay_and_status(delay, status)
    if err:
        return err

    user_id = request.state.user["sub"]
    return DataResponse(
        Success=True,
        Code="SUCCESS",
        Message="Data retrieved",
        Data=f"Hello, {user_id}!",
    )


@app.post("/api/data")
async def post_data(
    request: Request,
    delay: float | None = Query(None, description="응답 딜레이 (초)"),
    status: int | None = Query(None, description="시뮬레이션할 HTTP 상태코드"),
):
    err = await _apply_delay_and_status(delay, status)
    if err:
        return err

    body = await request.json()
    data = body.get("data", "")
    return DataResponse(
        Success=True,
        Code="SUCCESS",
        Message="Data created",
        Data=data,
    )


@app.put("/api/data")
async def put_data(
    request: Request,
    delay: float | None = Query(None, description="응답 딜레이 (초)"),
    status: int | None = Query(None, description="시뮬레이션할 HTTP 상태코드"),
):
    err = await _apply_delay_and_status(delay, status)
    if err:
        return err

    body = await request.json()
    data = body.get("data", "")
    return DataResponse(
        Success=True,
        Code="SUCCESS",
        Message="Data updated",
        Data=data,
    )


@app.delete("/api/data")
async def delete_data(
    request: Request,
    delay: float | None = Query(None, description="응답 딜레이 (초)"),
    status: int | None = Query(None, description="시뮬레이션할 HTTP 상태코드"),
):
    err = await _apply_delay_and_status(delay, status)
    if err:
        return err

    return BaseResponse(
        Success=True,
        Code="SUCCESS",
        Message="Data deleted",
    )


# ──────────────────────────────────────────────
# 디버그: 서버 상태 확인 (개발용)
# ──────────────────────────────────────────────

@app.post("/auth/reset")
def auth_reset():
    """모든 유저, 인증코드, 리프레시 토큰을 초기화."""
    user_count = len(user_store)
    verification_count = len(verification_store)
    refresh_count = len(refresh_token_store)

    user_store.clear()
    verification_store.clear()
    refresh_token_store.clear()

    return BaseResponse(
        Success=True,
        Code="RESET_SUCCESS",
        Message=f"Cleared {user_count} users, {verification_count} verifications, {refresh_count} refresh tokens",
    )


@app.get("/debug/users")
def debug_users():
    """등록된 유저 목록 (비밀번호 해시 제외)."""
    users = [
        {"email": email, "created_at": info["created_at"]}
        for email, info in user_store.items()
    ]
    return {
        "Success": True,
        "Code": "DEBUG",
        "Message": f"총 {len(users)}명",
        "Data": users,
    }


@app.get("/debug/verifications")
def debug_verifications():
    """현재 인증코드 상태 확인."""
    codes = [
        {
            "email": email,
            "code": info["code"],
            "verified": info["verified"],
            "remaining_seconds": max(0, int(VERIFICATION_CODE_EXPIRE_SECONDS - (time.time() - info["created_at"]))),
        }
        for email, info in verification_store.items()
    ]
    return {
        "Success": True,
        "Code": "DEBUG",
        "Message": f"총 {len(codes)}건",
        "Data": codes,
    }


# ──────────────────────────────────────────────
# 엔트리포인트
# ──────────────────────────────────────────────

if __name__ == "__main__":
    import uvicorn

    parser = argparse.ArgumentParser(description="JWNetworkUtility Test Server")
    parser.add_argument("--host", default="0.0.0.0", help="바인드 호스트 (기본: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=5000, help="바인드 포트 (기본: 5000)")
    parser.add_argument("--token-expire", type=int, default=60, help="엑세스 토큰 만료 시간(초) (기본: 60)")
    args = parser.parse_args()

    ACCESS_TOKEN_EXPIRE_SECONDS = args.token_expire

    uvicorn.run(app, host=args.host, port=args.port)
