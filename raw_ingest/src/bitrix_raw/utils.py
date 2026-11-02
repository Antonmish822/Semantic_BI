from __future__ import annotations

from datetime import datetime, timezone


def utc_date_str() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%d")
