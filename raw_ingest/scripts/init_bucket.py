from __future__ import annotations

import os

import boto3


def main() -> None:
    endpoint_url = os.environ.get("S3_ENDPOINT_URL", "").strip()
    access_key = os.environ.get("S3_ACCESS_KEY", "").strip()
    secret_key = os.environ.get("S3_SECRET_KEY", "").strip()
    bucket = os.environ.get("S3_BUCKET", "").strip()
    region = os.environ.get("S3_REGION", "us-east-1").strip()
    use_ssl = os.environ.get("S3_USE_SSL", "true").strip().lower() in {"1", "true", "yes", "y", "on"}

    if not endpoint_url or not access_key or not secret_key or not bucket:
        raise SystemExit("S3_ENDPOINT_URL, S3_ACCESS_KEY, S3_SECRET_KEY, S3_BUCKET are required")

    client = boto3.client(
        "s3",
        endpoint_url=endpoint_url,
        aws_access_key_id=access_key,
        aws_secret_access_key=secret_key,
        region_name=region,
        use_ssl=use_ssl,
    )

    existing = client.list_buckets().get("Buckets", [])
    if any(b["Name"] == bucket for b in existing):
        print(f"Bucket already exists: {bucket}")
        return

    client.create_bucket(Bucket=bucket)
    print(f"Bucket created: {bucket}")


if __name__ == "__main__":
    main()
