from __future__ import annotations
import boto3
import hashlib
import json
import re
from datetime import datetime, timezone
from typing import Any
from .config import Config
from .identity import SUPPORTED_ENTITIES

def canonical_json(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def make_s3(config: Config):
    return boto3.client(
        "s3", endpoint_url=config.endpoint_url, region_name=config.region,
        aws_access_key_id=config.access_key, aws_secret_access_key=config.secret_key,
    )


def get_object_bytes(s3, config: Config, key: str) -> bytes:
    return s3.get_object(Bucket=config.bucket, Key=key)["Body"].read()


def get_json(s3, config: Config, key: str, verify_metadata_sha: bool = False) -> dict[str, Any]:
    response = s3.get_object(Bucket=config.bucket, Key=key)
    body = response["Body"].read()
    if verify_metadata_sha:
        metadata = {str(name).lower(): value
                    for name, value in (response.get("Metadata") or {}).items()}
        expected_sha = metadata.get("sha256")
        actual_sha = hashlib.sha256(body).hexdigest()
        if not expected_sha or expected_sha != actual_sha:
            raise ValueError(f"SHA-256 metadata отсутствует или не совпадает для {key}")
    return json.loads(body.decode("utf-8-sig"))


def list_run_manifest_keys(s3, config: Config) -> list[str]:
    paginator = s3.get_paginator("list_objects_v2")
    keys = []
    for page in paginator.paginate(Bucket=config.bucket, Prefix=config.runs_prefix):
        for obj in page.get("Contents", []):
            key = obj["Key"]
            if key.endswith("/manifest.json"):
                keys.append(key)
    return sorted(keys)


def event_identity(event: dict[str, Any], portal_id: str, expected_type: str, expected_id: str) -> str:
    # Schema v4 adds optional deal payload fields while retaining the v3 event
    # envelope and entity identity contract consumed below.
    if event.get("schema_version") not in {3, 4}:
        raise ValueError(f"Неподдерживаемая schema_version: {event.get('schema_version')!r}")
    entity_key = event.get("entity_key")
    parts = entity_key.split("/", 2) if isinstance(entity_key, str) else []
    if len(parts) != 3 or parts[0] != portal_id or parts[1] != expected_type or parts[2] != expected_id:
        raise ValueError(f"entity_key не соответствует порталу/ключу S3: {entity_key!r}")
    return entity_key


def validate_event(event: dict[str, Any], ref: dict[str, Any], portal_id: str,
                   expected_type: str, expected_id: str) -> str:
    entity_key = event_identity(event, portal_id, expected_type, expected_id)
    if event.get("event_id") != ref.get("event_id") or event.get("graph_id") != ref.get("graph_id"):
        raise ValueError(f"Событие не совпадает с манифестом: {ref.get('key')}")
    expected_graph = f"urn:bitrix24:graph:{portal_id}:{event['event_id']}"
    if event.get("graph_id") != expected_graph:
        raise ValueError(f"Некорректный graph_id в событии: {ref.get('key')}")
    operation = event.get("operation")
    if operation not in {"upsert", "delete"}:
        raise ValueError(f"Неизвестная операция события: {operation!r}")
    if operation == "upsert":
        payload = event.get("payload")
        if not isinstance(payload, dict):
            raise ValueError("Для upsert требуется объект payload")
        identity = payload.get("ID", payload.get("STATUS_ID"))
        if expected_type in {"stage", "source"}:
            identity = f"{payload.get('ENTITY_ID')}:{payload.get('STATUS_ID')}"
        if str(identity) != expected_id:
            raise ValueError(f"ID payload не совпадает с entity_key для {entity_key}")
        bindings = event.get("bindings") or []
        digest = hashlib.sha256(canonical_json({"payload": payload, "bindings": bindings})).hexdigest()
        modified = payload.get("DATE_MODIFY") or payload.get("LAST_UPDATED") or payload.get("MODIFIED")
        computed_id = hashlib.sha256(
            f"{portal_id}|{expected_type}|{expected_id}|{modified}|{digest}".encode("utf-8")
        ).hexdigest()
        if computed_id != event["event_id"]:
            raise ValueError(f"event_id не подтверждается payload для {entity_key}")
    return entity_key


def parse_time(value: str | None) -> datetime | None:
    if not value:
        return None
    try:
        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
        if parsed.tzinfo is None:
            parsed = parsed.replace(tzinfo=timezone.utc)
        return parsed.astimezone(timezone.utc)
    except ValueError:
        return None


def run_id_time(run_id: str) -> datetime | None:
    try:
        return datetime.strptime(run_id[:16], "%Y%m%dT%H%M%SZ").replace(tzinfo=timezone.utc)
    except ValueError:
        return None


def is_stale(modified: str | None, current_modified: str | None) -> bool:
    incoming = parse_time(modified)
    current = parse_time(current_modified)
    return incoming is not None and current is not None and incoming < current


def not_newer_than(modified: str | None, boundary: str | None) -> bool:
    incoming = parse_time(modified)
    cutoff = parse_time(boundary)
    return incoming is not None and cutoff is not None and incoming <= cutoff


def validate_manifest(manifest: dict[str, Any], run_id: str, mode: str) -> list[dict[str, Any]]:
    if manifest.get("run_id") != run_id or manifest.get("mode") != mode:
        raise ValueError(f"run_id/mode не совпадает с ключом запуска {run_id}")
    if manifest.get("status") != "complete":
        raise ValueError(f"Манифест запуска {run_id} не complete")
    if mode == "local_snapshot" and manifest.get("complete") is not True:
        raise ValueError(f"Полный снимок {run_id} не помечен complete=true")
    if mode == "incremental":
        window = manifest.get("window") or {}
        start, end = parse_time(window.get("from")), parse_time(window.get("to"))
        if not start or not end or end <= start:
            raise ValueError(f"Некорректное окно incremental запуска {run_id}")
    refs = manifest.get("objects")
    if not isinstance(refs, list):
        raise ValueError(f"В манифесте {run_id} нет массива objects")
    if mode == "local_snapshot" and not refs:
        raise ValueError(f"Полный снимок {run_id} не содержит объектов")
    seen = set()
    for ref in refs:
        if not isinstance(ref, dict) or ref.get("key") in seen:
            raise ValueError(f"Некорректная или повторная ссылка в манифесте {run_id}")
        seen.add(ref.get("key"))
    if mode == "local_snapshot":
        counts: dict[str, int] = {}
        for ref in refs:
            entity = ref.get("entity")
            counts[entity] = counts.get(entity, 0) + 1
        if counts != manifest.get("counts"):
            raise ValueError(f"Counts снимка {run_id} не соответствуют objects")
    return refs


def read_exporter_state(s3, config: Config) -> dict[str, Any]:
    return get_json(s3, config, f"{config.portal_prefix}state.json")


def read_base_snapshot(s3, config: Config, exporter_state: dict[str, Any]):
    key = exporter_state.get("adopted_local_manifest_key")
    if not key:
        raise RuntimeError("Экспортёр ещё не принял полный локальный снимок")
    if not key.startswith(config.runs_prefix) or not key.endswith("/manifest.json"):
        raise ValueError("adopted_local_manifest_key выходит за каталог runs")
    manifest = get_json(s3, config, key)
    run_id = manifest.get("run_id")
    refs = validate_manifest(manifest, run_id, "local_snapshot")
    expected_sha = exporter_state.get("adopted_local_manifest_sha256")
    actual_sha = hashlib.sha256(canonical_json(manifest)).hexdigest()
    if not isinstance(expected_sha, str) or not re.fullmatch(r"[0-9a-f]{64}", expected_sha):
        raise ValueError("В state.json отсутствует SHA-256 принятого полного снимка")
    if expected_sha != actual_sha:
        raise ValueError("SHA-256 принятого полного снимка не совпадает с state.json")
    return key, manifest, refs
