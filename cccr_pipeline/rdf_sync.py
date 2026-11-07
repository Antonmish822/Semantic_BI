#!/usr/bin/env python3
"""Manifest-driven Bitrix24 S3 event synchronization to Jena Fuseki."""
from __future__ import annotations

import hashlib
import json
import logging
import fcntl
import re
import sqlite3
import time
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any
from urllib.parse import quote

import boto3
import requests
import yaml

try:
    import cccr_py as cccr
except ImportError as exc:  # pragma: no cover - environment-specific diagnostic
    raise SystemExit(
        "Не найден cccr_py. Установите собранную библиотеку CCCR в это Python-окружение."
    ) from exc

LOG = logging.getLogger("rdf_sync")
SUPPORTED_ENTITIES = {"deal", "activity", "user", "category", "stage", "source"}


@dataclass(frozen=True)
class Config:
    bucket: str
    portal_id: str
    endpoint_url: str
    region: str
    access_key: str
    secret_key: str
    fuseki_url: str
    fuseki_username: str | None
    fuseki_password: str | None
    graph_namespace: str
    registry_path: Path
    schemas_root: Path
    checkpoint_path: Path
    poll_interval: int
    reconcile_entity_types: frozenset[str]

    @property
    def portal_prefix(self) -> str:
        return f"bitrix24/{self.portal_id}/"

    @property
    def events_prefix(self) -> str:
        return f"{self.portal_prefix}events/"

    @property
    def runs_prefix(self) -> str:
        return f"{self.portal_prefix}runs/"


def load_config(path: Path) -> Config:
    raw = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    s3 = raw.get("s3") or {}
    fuseki = raw.get("fuseki") or {}
    mapping = raw.get("mapping") or {}
    checkpoint = raw.get("checkpoint") or {}
    missing = [key for key in ("bucket", "portal_id", "access_key", "secret_key") if not s3.get(key)]
    if missing:
        raise ValueError(f"Не заданы параметры s3: {', '.join(missing)}")
    if not fuseki.get("url"):
        raise ValueError("Не задан fuseki.url")
    if not mapping.get("registry_path") or not mapping.get("schemas_root"):
        raise ValueError("Задайте mapping.registry_path и mapping.schemas_root")

    def resolve(value: str) -> Path:
        candidate = Path(value).expanduser()
        return candidate.resolve() if candidate.is_absolute() else (path.parent / candidate).resolve()

    reconcile = frozenset(raw.get("snapshot_reconcile_entity_types", [
        "deal", "user", "category", "stage", "source",
    ]))
    unknown = reconcile - SUPPORTED_ENTITIES
    if unknown:
        raise ValueError(f"Неизвестные типы в snapshot_reconcile_entity_types: {sorted(unknown)}")

    return Config(
        bucket=s3["bucket"], portal_id=str(s3["portal_id"]),
        endpoint_url=s3.get("endpoint_url", "https://storage.yandexcloud.net"),
        region=s3.get("region", "ru-central1"), access_key=s3["access_key"],
        secret_key=s3["secret_key"], fuseki_url=fuseki["url"],
        fuseki_username=fuseki.get("username") or None,
        fuseki_password=fuseki.get("password") or None,
        graph_namespace=fuseki.get("graph_namespace", "urn:bitrix24:entity"),
        registry_path=resolve(mapping["registry_path"]),
        schemas_root=resolve(mapping["schemas_root"]),
        checkpoint_path=resolve(checkpoint.get("path", "state.sqlite3")),
        poll_interval=max(1, int(raw.get("poll_interval_seconds", 60))),
        reconcile_entity_types=reconcile,
    )


def canonical_json(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode("utf-8")


def entity_graph_uri(namespace: str, entity_key: str) -> str:
    parts = entity_key.split("/", 2)
    if len(parts) != 3 or not all(parts):
        raise ValueError(f"Некорректный entity_key: {entity_key!r}")
    portal, entity_type, _identity = parts
    if entity_type not in SUPPORTED_ENTITIES:
        raise ValueError(f"Неизвестный тип сущности в entity_key: {entity_type!r}")
    portal_part = quote(portal, safe="-._~")
    type_part = quote(entity_type, safe="-._~")
    digest = hashlib.sha256(entity_key.encode("utf-8")).hexdigest()
    return f"{namespace.rstrip(':') or namespace}:{portal_part}:{type_part}:{digest}"


def append_xml_value(parent: ET.Element, name: str, value: Any) -> None:
    node = ET.SubElement(parent, name)
    if value is None:
        node.set("nil", "true")
    elif isinstance(value, dict):
        for key, child in value.items():
            append_xml_value(node, str(key), child)
    elif isinstance(value, list):
        for child in value:
            append_xml_value(node, "item", child)
    elif isinstance(value, bool):
        node.text = "true" if value else "false"
    else:
        node.text = str(value)


def event_to_xml(event: dict[str, Any]) -> str:
    payload = event.get("payload")
    if not isinstance(payload, dict):
        raise ValueError("В событии отсутствует объект payload")
    root = ET.Element("json")
    record = ET.SubElement(root, "object")
    for field, value in payload.items():
        append_xml_value(record, field, value)
    ET.indent(root, space="  ")
    return ET.tostring(root, encoding="utf-8", xml_declaration=True).decode("utf-8")


@dataclass(frozen=True)
class MappingEntry:
    path: Path
    pattern: re.Pattern[str]
    match_mode: str
    key: str

    def matches(self, relative_key: str) -> bool:
        return getattr(self.pattern, self.match_mode)(relative_key) is not None


def load_mappings(config: Config) -> list[MappingEntry]:
    registry = json.loads(config.registry_path.read_text(encoding="utf-8"))
    if not isinstance(registry, list):
        raise ValueError("Mapping registry должен быть JSON-массивом")
    entries = []
    for index, item in enumerate(registry, 1):
        if not isinstance(item, dict):
            raise ValueError(f"Некорректная запись mapping registry №{index}")
        schema_name = item.get("manifest") or item.get("schema")
        pattern = item.get("pattern")
        if not isinstance(schema_name, str) or not isinstance(pattern, str):
            raise ValueError(f"В mapping registry №{index} нужны строки manifest/schema и pattern")
        schema_path = (config.schemas_root / schema_name).resolve()
        if not schema_path.is_relative_to(config.schemas_root) or not schema_path.is_file():
            raise ValueError(f"Файл схемы не найден или вышел за schemas_root: {schema_path}")
        flags = 0
        for flag in item.get("flags", ""):
            flags |= {"i": re.IGNORECASE, "m": re.MULTILINE, "s": re.DOTALL}.get(flag, 0)
        match_mode = item.get("match", "search")
        if match_mode not in {"search", "match", "fullmatch"}:
            raise ValueError(f"Неизвестный match mode: {match_mode!r}")
        entries.append(MappingEntry(schema_path, re.compile(pattern, flags), match_mode, schema_name))
    return entries


def mapping_fingerprint(config: Config, mappings: list[MappingEntry]) -> str:
    digest = hashlib.sha256()
    digest.update(config.registry_path.read_bytes())
    for entry in sorted(mappings, key=lambda value: value.key):
        digest.update(entry.key.encode("utf-8"))
        digest.update(entry.path.read_bytes())
    return digest.hexdigest()


class Checkpoint:
    def __init__(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        self.lock_file = path.with_suffix(path.suffix + ".lock").open("a+")
        try:
            fcntl.flock(self.lock_file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as exc:
            self.lock_file.close()
            raise RuntimeError(f"Другой загрузчик уже использует checkpoint {path}") from exc
        self.db = sqlite3.connect(path)
        self.db.row_factory = sqlite3.Row
        self.db.execute("PRAGMA journal_mode=WAL")
        self.db.executescript("""
            CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS entities (
                entity_key TEXT PRIMARY KEY,
                graph_uri TEXT NOT NULL,
                event_id TEXT NOT NULL,
                source_modified_at TEXT,
                source_key TEXT,
                sha256 TEXT,
                mapping_version TEXT,
                deleted INTEGER NOT NULL DEFAULT 0
            );
            CREATE TABLE IF NOT EXISTS runs (
                run_id TEXT PRIMARY KEY,
                manifest_key TEXT NOT NULL,
                created_at TEXT,
                mode TEXT NOT NULL
            );
        """)
        columns = {row[1] for row in self.db.execute("PRAGMA table_info(entities)")}
        for name, sql_type in (("source_key", "TEXT"), ("sha256", "TEXT"), ("mapping_version", "TEXT")):
            if name not in columns:
                self.db.execute(f"ALTER TABLE entities ADD COLUMN {name} {sql_type}")
        self.db.commit()

    def get_meta(self, key: str) -> str | None:
        row = self.db.execute("SELECT value FROM meta WHERE key=?", (key,)).fetchone()
        return row["value"] if row else None

    def set_meta(self, key: str, value: str) -> None:
        with self.db:
            self.db.execute("INSERT INTO meta(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value", (key, value))

    def get_entity(self, key: str):
        return self.db.execute("SELECT * FROM entities WHERE entity_key=?", (key,)).fetchone()

    def save_entity(self, key: str, uri: str, event_id: str, modified: str | None,
                    deleted: bool, source_key: str | None = None,
                    sha256: str | None = None, mapping_version: str | None = None) -> None:
        with self.db:
            self.db.execute("""INSERT INTO entities(entity_key,graph_uri,event_id,source_modified_at,
                source_key,sha256,mapping_version,deleted) VALUES(?,?,?,?,?,?,?,?)
                ON CONFLICT(entity_key) DO UPDATE SET graph_uri=excluded.graph_uri,
                event_id=excluded.event_id,source_modified_at=excluded.source_modified_at,
                source_key=COALESCE(excluded.source_key,entities.source_key),
                sha256=COALESCE(excluded.sha256,entities.sha256),
                mapping_version=COALESCE(excluded.mapping_version,entities.mapping_version),
                deleted=excluded.deleted""",
                (key, uri, event_id, modified, source_key, sha256, mapping_version, int(deleted)))

    def active_entities(self):
        return self.db.execute("SELECT * FROM entities WHERE deleted=0").fetchall()

    def run_done(self, run_id: str) -> bool:
        return self.db.execute("SELECT 1 FROM runs WHERE run_id=?", (run_id,)).fetchone() is not None

    def save_run(self, run_id: str, manifest_key: str, created_at: str | None, mode: str) -> None:
        with self.db:
            self.db.execute("INSERT OR IGNORE INTO runs(run_id,manifest_key,created_at,mode) VALUES(?,?,?,?)",
                            (run_id, manifest_key, created_at, mode))

    def entities_missing_from(self, present: set[str], entity_types: frozenset[str]):
        rows = self.db.execute("SELECT * FROM entities WHERE deleted=0").fetchall()
        for row in rows:
            parts = row["entity_key"].split("/", 2)
            if len(parts) == 3 and parts[1] in entity_types and row["entity_key"] not in present:
                yield row

    def close(self) -> None:
        self.db.close()
        fcntl.flock(self.lock_file.fileno(), fcntl.LOCK_UN)
        self.lock_file.close()


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
        expected_sha = (response.get("Metadata") or {}).get("sha256")
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
    if event.get("schema_version") != 3:
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


def fuseki_request(config: Config, method: str, graph_uri: str, body: bytes | None = None) -> None:
    endpoint = config.fuseki_url.rstrip("/") + "/data"
    auth = (config.fuseki_username, config.fuseki_password) if config.fuseki_username else None
    headers = {"Content-Type": "application/rdf+xml; charset=utf-8"} if body is not None else None
    response = requests.request(method, endpoint, params={"graph": graph_uri}, data=body,
                                headers=headers, auth=auth, timeout=90)
    if response.status_code not in ({"PUT": (200, 201, 204), "DELETE": (200, 204, 404)}[method]):
        raise RuntimeError(f"Fuseki {method} graph failed: HTTP {response.status_code}: {response.text[:500]}")


def transform_event(config: Config, mappings: list[MappingEntry], event: dict[str, Any],
                    relative_key: str) -> bytes:
    selected = [entry for entry in mappings if entry.matches(relative_key)]
    if not selected:
        raise ValueError(f"Нет CCCR mapping для {relative_key}")
    ctx = cccr.Context()
    xml_text = event_to_xml(event)
    source = ctx.load_xml(xml_text)
    if source is None:
        raise RuntimeError(f"CCCR не загрузил payload: {ctx.last_error}")
    combined = None
    for entry in selected:
        schema = ctx.load_schema(entry.path.read_text(encoding="utf-8"), include_base=str(entry.path.parent))
        if schema is None:
            raise RuntimeError(f"CCCR не загрузил схему {entry.path}: {ctx.last_error}")
        graph = ctx.run(schema, source)
        if graph is None:
            raise RuntimeError(f"CCCR не применил схему {entry.path}: {ctx.last_error}")
        if combined is None:
            combined = graph
        elif not combined.merge(graph):
            raise RuntimeError(f"Не удалось объединить результаты схем: {ctx.last_error}")
    rdfxml = combined.to_rdfxml()
    if rdfxml is None:
        raise RuntimeError(f"CCCR не сериализовал RDF/XML: {ctx.last_error}")
    return rdfxml.encode("utf-8")


def apply_event(s3, config: Config, checkpoint: Checkpoint, mappings: list[MappingEntry],
                ref: dict[str, Any], mapping_version: str,
                authoritative_snapshot: bool = False,
                baseline_modified_at: str | None = None) -> str:
    key = ref["key"]
    if not key.startswith(config.events_prefix):
        raise ValueError(f"Событие за пределами разрешённого префикса: {key}")
    relative_key = key[len(config.events_prefix):]
    path_parts = relative_key.split("/")
    if len(path_parts) != 3 or path_parts[0] not in SUPPORTED_ENTITIES:
        raise ValueError(f"Некорректный путь события: {key}")
    entity_type, identity, filename = path_parts
    event_id_from_key = filename[:-5] if filename.endswith(".json") else ""
    if event_id_from_key != ref.get("event_id"):
        raise ValueError(f"event_id в ключе не совпадает с манифестом: {key}")
    body = get_object_bytes(s3, config, key)
    actual_sha = hashlib.sha256(body).hexdigest()
    if actual_sha != ref.get("sha256"):
        raise ValueError(f"SHA-256 не совпадает с манифестом: {key}")
    event = json.loads(body.decode("utf-8-sig"))
    entity_key = validate_event(event, ref, config.portal_id, entity_type, identity)
    graph_uri = entity_graph_uri(config.graph_namespace, entity_key)
    previous = checkpoint.get_entity(entity_key)
    if (previous and previous["event_id"] == event["event_id"]
            and previous["graph_uri"] == graph_uri and not previous["deleted"]
            and previous["mapping_version"] == mapping_version):
        return entity_key
    modified = event.get("source_modified_at")
    if (previous and not authoritative_snapshot
            and is_stale(modified, previous["source_modified_at"])):
        LOG.warning("Пропущена более старая версия события для %s", entity_key)
        return entity_key
    if not authoritative_snapshot and not_newer_than(modified, baseline_modified_at):
        LOG.info("Событие %s не новее базового снимка; пропускаю", event["event_id"])
        return entity_key

    if event["operation"] == "delete":
        fuseki_request(config, "DELETE", graph_uri)
        checkpoint.save_entity(entity_key, graph_uri, event["event_id"], modified, True,
                               key, actual_sha, mapping_version)
    else:
        rdfxml = transform_event(config, mappings, event, relative_key)
        fuseki_request(config, "PUT", graph_uri, rdfxml)
        checkpoint.save_entity(entity_key, graph_uri, event["event_id"], modified, False,
                               key, actual_sha, mapping_version)
    LOG.info("Применено событие %s в граф %s", event["event_id"], graph_uri)
    return entity_key


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


def process_manifest(s3, config: Config, checkpoint: Checkpoint,
                     mappings: list[MappingEntry], manifest_key: str, manifest: dict[str, Any],
                     mode: str, mapping_version: str,
                     baseline_modified_at: str | None = None) -> set[str]:
    run_id = manifest["run_id"]
    refs = validate_manifest(manifest, run_id, mode)
    present: set[str] = set()
    for ref in refs:
        entity_key = apply_event(
            s3, config, checkpoint, mappings, ref, mapping_version,
            authoritative_snapshot=(mode == "local_snapshot"),
            baseline_modified_at=baseline_modified_at,
        )
        if mode == "local_snapshot" and entity_key in present:
            raise ValueError(f"Полный снимок содержит несколько версий {entity_key}")
        present.add(entity_key)
    if mode == "local_snapshot":
        for row in checkpoint.entities_missing_from(present, config.reconcile_entity_types):
            fuseki_request(config, "DELETE", row["graph_uri"])
            checkpoint.save_entity(row["entity_key"], row["graph_uri"],
                                   f"snapshot-delete:{run_id}", manifest.get("catchup_to"), True,
                                   mapping_version=mapping_version)
            LOG.info("Удалена сущность, отсутствующая в полном снимке: %s", row["entity_key"])
    checkpoint.save_run(run_id, manifest_key, manifest.get("created_at"), mode)
    return present


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


def maybe_apply_snapshot(s3, config: Config, checkpoint: Checkpoint,
                         mappings: list[MappingEntry], exporter_state: dict[str, Any],
                         mapping_version: str) -> tuple[str, bool, set[str]]:
    key, manifest, _refs = read_base_snapshot(s3, config, exporter_state)
    catchup_to = manifest.get("catchup_to")
    if not parse_time(catchup_to):
        raise ValueError(f"В полном снимке {key} некорректное поле catchup_to")
    changed = checkpoint.get_meta("base_manifest_key") != key
    present: set[str] = set()
    if changed:
        LOG.info("Применяется базовый снимок %s", key)
        present = process_manifest(s3, config, checkpoint, mappings, key, manifest,
                                   "local_snapshot", mapping_version)
        checkpoint.set_meta("base_manifest_key", key)
        checkpoint.set_meta("base_catchup_to", catchup_to)
        checkpoint.set_meta("last_run_id", "")
    return catchup_to, changed, present


def run_incrementals(s3, config: Config, checkpoint: Checkpoint,
                     mappings: list[MappingEntry], exporter_state: dict[str, Any],
                     catchup_to: str, mapping_version: str) -> None:
    latest_run_id = exporter_state.get("last_run_id")
    if not latest_run_id:
        return
    keys = list_run_manifest_keys(s3, config)
    available_run_ids = {key[len(config.runs_prefix):-len("/manifest.json")] for key in keys}
    if latest_run_id not in available_run_ids:
        raise RuntimeError(f"state.json указывает на отсутствующий манифест {latest_run_id}")
    checkpoint_run_id = checkpoint.get_meta("last_run_id")
    if checkpoint_run_id and checkpoint_run_id not in available_run_ids:
        raise RuntimeError(
            f"Checkpoint указывает на истёкший/отсутствующий запуск {checkpoint_run_id}; "
            "нужен новый полный снимок для безопасной синхронизации"
        )
    if checkpoint_run_id and checkpoint_run_id > latest_run_id:
        raise RuntimeError("Checkpoint опережает exporter state.json; проверьте восстановление S3/state")
    if checkpoint_run_id:
        lower_bound = checkpoint_run_id
        cursor_time = run_id_time(checkpoint_run_id)
    else:
        catchup = parse_time(catchup_to)
        if not catchup:
            raise ValueError("Checkpoint snapshot не содержит корректную границу catchup_to")
        lower_bound = catchup.strftime("%Y%m%dT%H%M%SZ")
        cursor_time = catchup
    latest_time = run_id_time(latest_run_id)
    if cursor_time and latest_time and latest_time - cursor_time >= timedelta(days=85):
        raise RuntimeError(
            "RDF checkpoint отстаёт почти на срок хранения событий S3; "
            "сначала опубликуйте и примите новый полный снимок"
        )
    candidates = []
    for key in keys:
        run_id = key[len(config.runs_prefix):-len("/manifest.json")]
        if run_id.startswith("local-") or not lower_bound < run_id <= latest_run_id:
            continue
        manifest = get_json(s3, config, key, verify_metadata_sha=True)
        mode = manifest.get("mode")
        if run_id == latest_run_id and mode != "incremental":
            raise ValueError(f"last_run_id {run_id} не указывает на incremental манифест")
        if mode == "backfill":
            continue
        if mode != "incremental":
            continue
        candidates.append((key, run_id, manifest))
    # Exporter run IDs start with UTC timestamps; manifest timestamps are the
    # canonical ordering where available.
    candidates.sort(key=lambda item: (
        item[2].get("window", {}).get("to", ""),
        item[2].get("created_at", ""), item[1],
    ))
    catchup = parse_time(catchup_to)
    for key, run_id, manifest in candidates:
        window_to = parse_time((manifest.get("window") or {}).get("to"))
        if catchup and window_to and window_to <= catchup:
            checkpoint.save_run(run_id, key, manifest.get("created_at"), "incremental")
            continue
        status_key = key[:-len("manifest.json")] + "status.json"
        try:
            status = get_json(s3, config, status_key)
        except Exception as exc:
            LOG.info("Ожидается статус запуска %s (%s); последующие запуски пока не применяются", run_id, exc)
            break
        if status.get("run_id") != run_id or status.get("status") != "complete":
            LOG.warning("Запуск %s не завершён; последующие манифесты отложены", run_id)
            break
        validate_manifest(manifest, run_id, "incremental")
        process_manifest(s3, config, checkpoint, mappings, key, manifest,
                         "incremental", mapping_version, catchup_to)
        checkpoint.set_meta("last_run_id", run_id)
        LOG.info("Применён инкрементальный запуск %s", run_id)
        if run_id == latest_run_id:
            break


def run_cycle(s3, config: Config, checkpoint: Checkpoint, mappings: list[MappingEntry]) -> None:
    exporter_state = read_exporter_state(s3, config)
    current_mapping_version = mapping_fingerprint(config, mappings)
    catchup_to, _snapshot_changed, snapshot_entities = maybe_apply_snapshot(
        s3, config, checkpoint, mappings, exporter_state, current_mapping_version
    )
    old_mapping_version = checkpoint.get_meta("mapping_version")
    if old_mapping_version and old_mapping_version != current_mapping_version:
        LOG.info("Версия CCCR mappings изменилась; пересобираются текущие графы")
        for row in checkpoint.active_entities():
            if row["entity_key"] in snapshot_entities:
                continue
            ref = {
                "entity": row["entity_key"].split("/", 2)[1],
                "event_id": row["event_id"], "graph_id": f"urn:bitrix24:graph:{config.portal_id}:{row['event_id']}",
                "key": row["source_key"], "sha256": row["sha256"],
            }
            apply_event(s3, config, checkpoint, mappings, ref, current_mapping_version)
    run_incrementals(s3, config, checkpoint, mappings, exporter_state,
                     catchup_to, current_mapping_version)
    checkpoint.set_meta("mapping_version", current_mapping_version)


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(description="Bitrix24 S3 manifests → named Fuseki graphs")
    parser.add_argument("--config", type=Path, default=Path(__file__).with_name("config.yaml"))
    parser.add_argument("--once", action="store_true", help="Выполнить один цикл")
    parser.add_argument("--log-level", default="INFO")
    args = parser.parse_args()
    logging.basicConfig(level=getattr(logging, args.log_level.upper(), logging.INFO),
                        format="%(asctime)s [%(levelname)s] %(message)s")
    config_path = args.config.resolve()
    config = load_config(config_path)
    mappings = load_mappings(config)
    s3 = make_s3(config)
    checkpoint = Checkpoint(config.checkpoint_path)
    try:
        while True:
            try:
                run_cycle(s3, config, checkpoint, mappings)
            except Exception:
                LOG.exception("Ошибка цикла синхронизации; checkpoint сохранён для повтора")
                if args.once:
                    raise
            if args.once:
                return
            time.sleep(config.poll_interval)
    finally:
        checkpoint.close()


if __name__ == "__main__":
    main()
