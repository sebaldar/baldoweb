"""Service-to-service authorization for administrative routes."""
import hmac
import os

from fastapi import Header, HTTPException


def require_admin(authorization: str = Header(default="")) -> None:
    expected = os.environ.get("ADMIN_TOKEN", "")
    supplied = authorization.removeprefix("Bearer ") if authorization.startswith("Bearer ") else ""
    if len(expected) < 32 or not hmac.compare_digest(supplied.encode(), expected.encode()):
        raise HTTPException(status_code=403, detail="Accesso amministratore richiesto")
