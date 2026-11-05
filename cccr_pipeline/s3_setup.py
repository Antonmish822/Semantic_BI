#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Небольшая утилита для первоначальной настройки бакета:
- создаёт "папки" (пустые объекты-маркеры вида "имя/")
- показывает содержимое бакета

Использует те же учётные данные, что и config.yaml основного пайплайна.
Запуск:
    python3 s3_setup.py mkdir incoming/
    python3 s3_setup.py mkdir manifests/
    python3 s3_setup.py ls
    python3 s3_setup.py upload local_file.json manifests/registry.json
"""

from __future__ import annotations

import sys
from pathlib import Path

import boto3
import yaml

CONFIG_PATH = Path(__file__).resolve().parent / "config.yaml"


def load_s3_client():
    raw = yaml.safe_load(CONFIG_PATH.read_text(encoding="utf-8"))
    s3_raw = raw["s3"]
    session = boto3.session.Session()
    client = session.client(
        "s3",
        endpoint_url=s3_raw.get("endpoint_url", "https://storage.yandexcloud.net"),
        region_name=s3_raw.get("region", "ru-central1"),
        aws_access_key_id=s3_raw["key_id"],
        aws_secret_access_key=s3_raw["secret_key"],
    )
    return client, s3_raw["bucket"]


def mkdir(client, bucket, folder: str):
    if not folder.endswith("/"):
        folder += "/"
    client.put_object(Bucket=bucket, Key=folder, Body=b"")
    print(f"Создана папка: {folder}")


def ls(client, bucket, prefix: str = ""):
    paginator = client.get_paginator("list_objects_v2")
    found = False
    for page in paginator.paginate(Bucket=bucket, Prefix=prefix):
        for obj in page.get("Contents", []):
            found = True
            print(f"{obj['LastModified']}  {obj['Size']:>10}  {obj['Key']}")
    if not found:
        print("(пусто)")


def upload(client, bucket, local_path: str, key: str):
    client.upload_file(local_path, bucket, key)
    print(f"Загружено: {local_path} -> s3://{bucket}/{key}")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        raise SystemExit(1)

    client, bucket = load_s3_client()
    cmd = sys.argv[1]

    if cmd == "mkdir" and len(sys.argv) == 3:
        mkdir(client, bucket, sys.argv[2])
    elif cmd == "ls":
        ls(client, bucket, sys.argv[2] if len(sys.argv) > 2 else "")
    elif cmd == "upload" and len(sys.argv) == 4:
        upload(client, bucket, sys.argv[2], sys.argv[3])
    else:
        print(__doc__)
        raise SystemExit(1)


if __name__ == "__main__":
    main()
