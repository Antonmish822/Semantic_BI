"""Contract 2 validation, intentionally independent from exporter runtime code."""
from __future__ import annotations

import hashlib
import json
import re
from urllib.parse import quote_from_bytes, unquote_to_bytes
from collections import Counter
from datetime import datetime, timezone
from typing import Any

from ..identity import SUPPORTED_ENTITIES

HEX = re.compile(r"^[0-9a-f]{64}$")
RUN = re.compile(r"^(?:local-)?\d{8}T\d{6}Z-[A-Za-z0-9_-]+$")


def canonical_json(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"),
                      ensure_ascii=False, allow_nan=False).encode("utf-8")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def encode_component(value: Any) -> str:
    if isinstance(value, bool) or not isinstance(value, (str, int)):
        raise ValueError("Компонент identity должен быть строкой или целым числом")
    raw = str(value)
    if not raw:
        raise ValueError("Компонент identity не может быть пустым")
    return quote_from_bytes(raw.encode("utf-8"), safe="-._~")


def decode_component(value: str) -> str:
    try:
        decoded = unquote_to_bytes(value).decode("utf-8")
    except (UnicodeDecodeError, ValueError) as exc:
        raise ValueError("Некорректное percent-кодирование identity") from exc
    if encode_component(decoded) != value:
        raise ValueError("Неканоническое percent-кодирование identity")
    return decoded


def normalize_timestamp(value: Any) -> str | None:
    if value is None or value == "":
        return None
    dt = datetime.fromisoformat(str(value).replace("Z", "+00:00"))
    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=timezone.utc)
    dt = dt.astimezone(timezone.utc)
    base = dt.strftime("%Y-%m-%dT%H:%M:%S")
    return f"{base}.{dt.microsecond:06d}Z" if dt.microsecond else base + "Z"


def object_json(body: bytes, label: str) -> dict[str, Any]:
    try:
        value = json.loads(body.decode("utf-8"), parse_constant=lambda x: (_ for _ in ()).throw(ValueError(x)))
    except (UnicodeDecodeError, json.JSONDecodeError, ValueError) as exc:
        raise ValueError(f"Некорректный JSON объекта {label}") from exc
    if not isinstance(value, dict):
        raise ValueError(f"Ожидался JSON объект: {label}")
    if canonical_json(value) != body:
        raise ValueError(f"Неканонические JSON байты: {label}")
    return value


def validate_state(state: dict[str, Any], portal_prefix: str) -> None:
    if not re.fullmatch(r"bitrix24/[A-Za-z0-9_.-]+/", portal_prefix):
        raise ValueError("Некорректный portal prefix загрузчика")
    required = {"contract_version", "revision", "epoch_id", "base_manifest_key",
                "base_manifest_sha256", "tip_run_id", "tip_manifest_sha256"}
    if not required.issubset(state) or type(state["contract_version"]) is not int or state["contract_version"] != 2:
        raise ValueError("state.json не соответствует контракту 2")
    if type(state["revision"]) is not int or state["revision"] < 1:
        raise ValueError("Некорректная revision state.json")
    epoch = state["epoch_id"]
    if not isinstance(epoch, str) or not epoch.startswith("local-") or not RUN.fullmatch(epoch):
        raise ValueError("Некорректная epoch_id state.json")
    base_key = state["base_manifest_key"]
    if base_key != f"{portal_prefix}runs/{epoch}/manifest.json":
        raise ValueError("base_manifest_key выходит за portal/epoch")
    if not isinstance(state["base_manifest_sha256"], str) or not HEX.fullmatch(state["base_manifest_sha256"]):
        raise ValueError("Некорректный SHA базового манифеста")
    tip, tip_sha = state["tip_run_id"], state["tip_manifest_sha256"]
    if (tip is None) != (tip_sha is None):
        raise ValueError("Указатель tip должен быть полностью null или заполнен")
    if tip is not None and (not isinstance(tip, str) or not re.fullmatch(r"\d{8}T\d{6}Z-[A-Za-z0-9_-]+", tip)
                            or not isinstance(tip_sha, str) or not HEX.fullmatch(tip_sha)):
        raise ValueError("Некорректная вершина цепочки state.json")


def validate_manifest(manifest: dict[str, Any], portal_prefix: str) -> None:
    if type(manifest.get("contract_version")) is not int or manifest["contract_version"] != 2:
        raise ValueError("manifest contract_version должен быть 2")
    run_id, mode, epoch = manifest.get("run_id"), manifest.get("mode"), manifest.get("epoch_id")
    if not isinstance(run_id, str) or not RUN.fullmatch(run_id) or mode not in {"incremental", "backfill", "local_snapshot"}:
        raise ValueError("Некорректная идентичность манифеста")
    if (mode == "local_snapshot") != run_id.startswith("local-"):
        raise ValueError("run_id не соответствует mode")
    if not isinstance(epoch, str) or not epoch.startswith("local-") or not RUN.fullmatch(epoch):
        raise ValueError("Некорректная epoch_id манифеста")
    if mode == "local_snapshot" and (epoch != run_id or manifest.get("previous_run_id") is not None
                                      or manifest.get("previous_manifest_sha256") is not None):
        raise ValueError("Некорректная базовая snapshot эпохи")
    prev, prev_sha = manifest.get("previous_run_id"), manifest.get("previous_manifest_sha256")
    if (prev is None) != (prev_sha is None):
        raise ValueError("Предыдущий run и SHA должны быть заданы парой")
    if prev is not None and (mode != "incremental" or not isinstance(prev, str) or not RUN.fullmatch(prev)
                             or prev.startswith("local-") or prev == run_id
                             or not isinstance(prev_sha, str) or not HEX.fullmatch(prev_sha)):
        raise ValueError("Некорректная ссылка предыдущего incremental")
    counts = manifest.get("counts")
    if not isinstance(counts, dict) or set(counts) != set(SUPPORTED_ENTITIES):
        raise ValueError("counts должны содержать шесть типов сущностей")
    if any(type(counts[e]) is not int or counts[e] < 0 for e in SUPPORTED_ENTITIES):
        raise ValueError("counts должны быть неотрицательными целыми числами")
    refs = manifest.get("objects")
    if not isinstance(refs, list):
        raise ValueError("В манифесте отсутствует objects")
    actual, seen = Counter(), set()
    for ref in refs:
        if not isinstance(ref, dict) or set(ref) != {"entity", "event_id", "graph_id", "key", "sha256"}:
            raise ValueError("Некорректная ссылка на событие")
        entity, event_id, key, digest = ref["entity"], ref["event_id"], ref["key"], ref["sha256"]
        if entity not in SUPPORTED_ENTITIES or not isinstance(event_id, str) or not HEX.fullmatch(event_id):
            raise ValueError("Некорректная идентичность ссылки")
        if not isinstance(digest, str) or not HEX.fullmatch(digest) or key in seen:
            raise ValueError("SHA неверен или ключ повторяется")
        expected_prefix = f"{portal_prefix}events/{entity}/"
        if (not isinstance(key, str) or not key.startswith(expected_prefix) or key.count("/") != 5
                or not key.endswith(f"/{event_id}.json")
                or ref["graph_id"] != f"urn:bitrix24:graph:{portal_prefix.split('/')[1]}:{event_id}"):
            raise ValueError("Ключ или graph_id ссылки не соответствует порталу")
        seen.add(key)
        actual[entity] += 1
    if any(counts[e] != actual[e] for e in SUPPORTED_ENTITIES):
        raise ValueError("counts манифеста не совпадают с objects")
    if manifest.get("status") != "complete":
        raise ValueError("Манифест не complete")
    if mode == "local_snapshot":
        complete = manifest.get("complete_entity_types")
        if manifest.get("complete") is not True or not isinstance(complete, list):
            raise ValueError("Snapshot не объявлен полным")
        allowed = set(SUPPORTED_ENTITIES) - {"activity"}
        if any(e not in allowed for e in complete) or len(set(complete)) != len(complete):
            raise ValueError("Недопустимый complete_entity_types")
        for field in ("t0", "catchup_to", "activity_from"):
            if not isinstance(manifest.get(field), str):
                raise ValueError(f"В snapshot отсутствует {field}")
            normalize_timestamp(manifest[field])
        if datetime.fromisoformat(manifest["catchup_to"].replace("Z", "+00:00")) < datetime.fromisoformat(manifest["t0"].replace("Z", "+00:00")):
            raise ValueError("catchup_to предшествует t0")
    else:
        window = manifest.get("window")
        if not isinstance(window, dict) or not isinstance(window.get("from"), str) or not isinstance(window.get("to"), str):
            raise ValueError("Окно запуска отсутствует")
        if normalize_timestamp(window["to"]) <= normalize_timestamp(window["from"]):
            raise ValueError("Некорректное окно запуска")


def validate_event(event: dict[str, Any], ref: dict[str, Any], portal: str) -> str:
    expected_fields = {"schema_version", "event_id_algorithm", "event_id", "graph_id",
                       "entity_key", "operation", "source_modified_at", "observed_at",
                       "payload", "bindings"}
    if set(event) != expected_fields:
        raise ValueError("Набор полей event envelope не соответствует контракту 2")
    algorithm = event.get("event_id_algorithm")
    if event.get("schema_version") != 4 or algorithm not in (3, 4):
        raise ValueError("Неподдерживаемая версия события")
    payload, bindings = event.get("payload"), event.get("bindings")
    if not isinstance(payload, dict) or not isinstance(bindings, list) or event.get("operation") != "upsert":
        raise ValueError("Некорректная операция или payload события")
    entity = ref["entity"]
    if entity in {"stage", "source"}:
        a, b = payload.get("ENTITY_ID"), payload.get("STATUS_ID")
        if a is None or b is None:
            raise ValueError("Составная сущность не содержит identity")
        identity = (f"{encode_component(a)}:{encode_component(b)}" if algorithm == 4
                    else f"{a}:{b}")
    else:
        value = payload.get("ID", payload.get("id"))
        if value is None:
            raise ValueError("Событие не содержит identity")
        identity = encode_component(value) if algorithm == 4 else str(value)
    if algorithm == 3 and any("|" in part or "/" in part for part in (portal, entity, identity)):
        raise ValueError("Недопустимый разделитель в legacy identity")
    if algorithm == 4:
        if entity in {"stage", "source"}:
            components = identity.split(":")
            if len(components) != 2 or any(not c for c in components):
                raise ValueError("Составная identity должна содержать два компонента")
            for component in components:
                decode_component(component)
        else:
            decode_component(identity)
    entity_key = f"{portal}/{entity}/{identity}"
    expected_key = f"{portal_prefix_for(portal)}events/{entity}/{identity}/{event.get('event_id')}.json"
    if event.get("entity_key") != entity_key or ref["key"] != expected_key:
        raise ValueError("entity_key или ключ события не соответствует identity")
    modified = next((payload.get(name) for name in ("DATE_MODIFY", "LAST_UPDATED", "MODIFIED")
                     if payload.get(name) not in (None, "")), None)
    stamp = normalize_timestamp(modified)
    if event.get("source_modified_at") != stamp:
        raise ValueError("source_modified_at не соответствует payload")
    if event.get("observed_at") != (stamp or "1970-01-01T00:00:00Z"):
        raise ValueError("observed_at не соответствует source_modified_at")
    normalized_bindings = []
    for item in bindings:
        if not isinstance(item, dict) or set(item) != {"entityTypeId", "entityId"}:
            raise ValueError("Некорректная activity binding")
        normalized_bindings.append(item)
    if entity != "activity" and bindings:
        raise ValueError("bindings разрешены только для activity")
    if normalized_bindings != sorted(normalized_bindings, key=lambda x: (str(x["entityTypeId"]), str(x["entityId"]))):
        raise ValueError("activity bindings не нормализованы")
    digest = sha256(canonical_json({"event_id_algorithm": algorithm, "schema_version": 4,
                                    "payload": payload, "bindings": bindings}))
    if algorithm == 3:
        token = stamp if stamp is not None else "<null>"
        calculated = sha256(f"{portal}|{entity}|{identity}|{token}|{digest}".encode("utf-8"))
    else:
        calculated = sha256(canonical_json(["bitrix24-event-id-v4", portal, entity, identity, stamp, digest]))
    if event.get("event_id") != calculated or event.get("event_id") != ref.get("event_id"):
        raise ValueError(f"event_id не подтверждается алгоритмом {algorithm}")
    if event.get("graph_id") != f"urn:bitrix24:graph:{portal}:{calculated}" or ref.get("graph_id") != event.get("graph_id"):
        raise ValueError("graph_id не соответствует event_id")
    return entity_key


def portal_prefix_for(portal: str) -> str:
    return f"bitrix24/{portal}/"
