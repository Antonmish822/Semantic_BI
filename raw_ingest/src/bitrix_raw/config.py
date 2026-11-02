from __future__ import annotations

import os
from dataclasses import dataclass


def _get_bool(value: str | None, default: bool) -> bool:
    if value is None:
        return default
    return value.strip().lower() in {"1", "true", "yes", "y", "on"}


@dataclass(frozen=True)
class Settings:
    bitrix_webhook_url: str
    s3_endpoint_url: str
    s3_access_key: str
    s3_secret_key: str
    s3_bucket: str
    s3_prefix: str
    s3_region: str
    s3_use_ssl: bool


def load_settings() -> Settings:
    bitrix_webhook_url = os.environ.get("BITRIX_WEBHOOK_URL", "").strip()
    if not bitrix_webhook_url:
        raise ValueError("BITRIX_WEBHOOK_URL is required")

    s3_endpoint_url = os.environ.get("S3_ENDPOINT_URL", "").strip()
    if not s3_endpoint_url:
        raise ValueError("S3_ENDPOINT_URL is required")

    s3_access_key = os.environ.get("S3_ACCESS_KEY", "").strip()
    s3_secret_key = os.environ.get("S3_SECRET_KEY", "").strip()
    s3_bucket = os.environ.get("S3_BUCKET", "").strip()

    if not s3_access_key or not s3_secret_key or not s3_bucket:
        raise ValueError("S3_ACCESS_KEY, S3_SECRET_KEY, S3_BUCKET are required")

    s3_prefix = os.environ.get("S3_PREFIX", "raw/bitrix24").strip()
    s3_region = os.environ.get("S3_REGION", "us-east-1").strip()
    s3_use_ssl = _get_bool(os.environ.get("S3_USE_SSL"), default=True)

    return Settings(
        bitrix_webhook_url=bitrix_webhook_url,
        s3_endpoint_url=s3_endpoint_url,
        s3_access_key=s3_access_key,
        s3_secret_key=s3_secret_key,
        s3_bucket=s3_bucket,
        s3_prefix=s3_prefix,
        s3_region=s3_region,
        s3_use_ssl=s3_use_ssl,
    )
