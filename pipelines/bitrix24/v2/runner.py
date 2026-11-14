"""State-anchored contract 2 synchronization path."""
from __future__ import annotations

import logging
import time
from datetime import datetime
from botocore.exceptions import ClientError

from ...core.runner import apply_graph_operation
from ..identity import SUPPORTED_ENTITIES, entity_graph_uri
from ..mapper import mapping_fingerprint, transform_event
from .protocol import (canonical_json, object_json, sha256, validate_event,
                       validate_manifest, validate_state)

LOG = logging.getLogger("rdf_sync")


def _ensure_deadline(deadline):
    if deadline is not None and time.monotonic() >= deadline:
        raise TimeoutError("Превышено max_cycle_read_seconds")


def _read(s3, config, key: str, maximum: int, deadline=None) -> bytes:
    _ensure_deadline(deadline)
    if not key.startswith(config.portal_prefix):
        raise ValueError("Ключ объекта выходит за portal prefix")
    response = s3.get_object(Bucket=config.bucket, Key=key)
    body = response["Body"].read(maximum + 1)
    _ensure_deadline(deadline)
    if len(body) > maximum:
        raise ValueError(f"Превышен допустимый размер объекта {key}")
    metadata = {str(k).lower(): v for k, v in (response.get("Metadata") or {}).items()}
    if metadata.get("sha256") and metadata["sha256"] != sha256(body):
        raise ValueError(f"SHA-256 metadata не совпадает для {key}")
    return body


def read_contract_state(s3, config, deadline=None) -> tuple[dict, str]:
    key = f"{config.portal_prefix}state.json"
    body = _read(s3, config, key, config.max_state_bytes, deadline)
    state = object_json(body, key)
    validate_state(state, config.portal_prefix)
    return state, sha256(body)


def _manifest(s3, config, key: str, expected_sha: str | None, deadline=None):
    body = _read(s3, config, key, config.max_manifest_bytes, deadline)
    actual = sha256(body)
    if expected_sha is not None and actual != expected_sha:
        raise ValueError(f"SHA-256 манифеста не совпадает: {key}")
    manifest = object_json(body, key)
    validate_manifest(manifest, config.portal_prefix)
    if key != f"{config.runs_prefix}{manifest['run_id']}/manifest.json":
        raise ValueError(f"Ключ манифеста не соответствует run_id: {key}")
    return manifest, actual


def _base_snapshot(s3, config, state, deadline=None):
    manifest, digest = _manifest(s3, config, state["base_manifest_key"], state["base_manifest_sha256"], deadline)
    if manifest["mode"] != "local_snapshot" or manifest["epoch_id"] != state["epoch_id"]:
        raise ValueError("Базовый манифест не соответствует epoch state.json")
    return manifest, digest


def _timestamp(value):
    if value is None:
        return None
    return datetime.fromisoformat(value.replace("Z", "+00:00"))


def _chain(s3, config, state, checkpoint, deadline=None):
    """Read and verify the whole linked ancestry before applying any run."""
    tip = state["tip_run_id"]
    checkpoint_raw_id = checkpoint.get_meta("applied_tip_run_id")
    if tip is None:
        if checkpoint.get_meta("epoch_id") == state["epoch_id"] and checkpoint_raw_id is None:
            raise RuntimeError("Checkpoint epoch не содержит согласованной пустой вершины")
        if checkpoint.get_meta("epoch_id") == state["epoch_id"] and checkpoint_raw_id:
            raise RuntimeError("state.json tip отстаёт от v2 checkpoint; требуется проверка/восстановление")
        return []
    cursor_id, cursor_sha = tip, state["tip_manifest_sha256"]
    visited, reverse = set(), []
    child_window_to = None
    checkpoint_run_id = checkpoint.get_meta("applied_tip_run_id") or None
    matched_checkpoint = checkpoint_run_id is None and checkpoint_raw_id == ""
    max_chain = config.max_chain_length
    object_count = 0
    while cursor_id is not None:
        if cursor_id in visited:
            raise ValueError("Обнаружен цикл в подтверждённой цепочке")
        if len(reverse) >= max_chain:
            raise ValueError(f"Цепочка превышает checkpoint.max_chain_length={max_chain}")
        visited.add(cursor_id)
        key = f"{config.runs_prefix}{cursor_id}/manifest.json"
        manifest, digest = _manifest(s3, config, key, cursor_sha, deadline)
        object_count += len(manifest["objects"])
        if object_count > config.max_objects_per_cycle:
            raise ValueError(f"Цепочка превышает contract_v2_limits.max_objects_per_cycle={config.max_objects_per_cycle}")
        if (manifest["run_id"] != cursor_id or manifest["mode"] != "incremental"
                or manifest["epoch_id"] != state["epoch_id"]):
            raise ValueError(f"Звено цепочки не соответствует epoch/run: {cursor_id}")
        window_to = _timestamp(manifest["window"]["to"])
        if child_window_to is not None and window_to > child_window_to:
            raise ValueError("window.to убывает при обратном обходе цепочки")
        child_window_to = window_to
        reverse.append((key, manifest, digest))
        if (checkpoint_run_id == cursor_id
                and checkpoint.get_meta("applied_tip_manifest_sha256") == digest
                and checkpoint.get_meta("epoch_id") == state["epoch_id"]):
            matched_checkpoint = True
            break
        cursor_id = manifest["previous_run_id"]
        cursor_sha = manifest["previous_manifest_sha256"]
    checkpoint_id = checkpoint_run_id
    if checkpoint_id is not None and not matched_checkpoint:
        raise RuntimeError("Checkpoint v2 не является предком подтверждённой tip-цепочки; нужен snapshot или восстановление")
    if (checkpoint_id is None and checkpoint.get_meta("epoch_id") == state["epoch_id"]
            and checkpoint_raw_id is None):
        raise RuntimeError("Checkpoint epoch существует без согласованной вершины цепочки")
    return list(reversed(reverse))


def _verify_ref(s3, config, ref, deadline=None):
    _ensure_deadline(deadline)
    key = ref["key"]
    if not key.startswith(config.events_prefix):
        raise ValueError("Событие выходит за events prefix")
    relative = key[len(config.events_prefix):].split("/")
    if len(relative) != 3 or relative[0] != ref["entity"] or not relative[2].endswith(".json"):
        raise ValueError("Некорректный путь события")
    identity, event_id = relative[1], relative[2][:-5]
    body = _read(s3, config, key, config.max_event_bytes, deadline)
    actual_sha = sha256(body)
    if actual_sha != ref["sha256"]:
        raise ValueError(f"SHA-256 события не совпадает: {key}")
    event = object_json(body, key)
    entity_key = validate_event(event, ref, config.portal_id)
    return event, entity_key, actual_sha


def _apply_ref(s3, config, checkpoint, mappings, ref, mapping_version, store, deadline=None):
    _ensure_deadline(deadline)
    key = ref["key"]
    if not key.startswith(config.events_prefix):
        raise ValueError("Событие выходит за events prefix")
    relative = key[len(config.events_prefix):].split("/")
    if len(relative) != 3 or relative[0] != ref["entity"] or not relative[2].endswith(".json"):
        raise ValueError("Некорректный путь события")
    identity = relative[1]
    event_id = relative[2][:-5]
    entity_key = f"{config.portal_id}/{relative[0]}/{identity}"
    graph_uri = entity_graph_uri(config.graph_namespace, entity_key)
    previous = checkpoint.get_entity(entity_key)
    if (config.fast_resume and previous and previous["source_key"] == key
            and previous["sha256"] == ref["sha256"] and previous["event_id"] == event_id
            and ref["event_id"] == event_id
            and ref["graph_id"] == f"urn:bitrix24:graph:{config.portal_id}:{event_id}"
            and previous["graph_uri"] == graph_uri and previous["mapping_version"] == mapping_version
            and not previous["deleted"]):
        return entity_key
    event, entity_key, actual_sha = _verify_ref(s3, config, ref, deadline)
    graph_uri = entity_graph_uri(config.graph_namespace, entity_key)
    previous = checkpoint.get_entity(entity_key)
    if (previous and previous["source_key"] == key and previous["sha256"] == actual_sha
            and previous["event_id"] == event["event_id"]
            and previous["graph_uri"] == graph_uri
            and previous["mapping_version"] == mapping_version and not previous["deleted"]):
        return entity_key
    try:
        rdfxml = transform_event(config, mappings, event, key[len(config.events_prefix):])
    except Exception:
        raise RuntimeError("CCCR projection failed for validated event; payload omitted") from None
    try:
        apply_graph_operation(store, "replace", graph_uri,
            lambda: checkpoint.save_entity(entity_key, graph_uri, event["event_id"],
                                           event["source_modified_at"], False, key,
                                           actual_sha, mapping_version), rdfxml)
    except Exception:
        raise RuntimeError("RDF PUT or entity checkpoint write failed; payload omitted") from None
    return entity_key


def _apply_manifest(s3, config, checkpoint, mappings, entry, mapping_version, store, deadline=None):
    key, manifest, digest = entry
    if len(manifest["objects"]) > config.max_objects_per_run:
        raise ValueError(f"Манифест превышает max_objects_per_run: {key}")
    present = set()
    if len(manifest["objects"]) > config.max_objects_per_cycle:
        raise ValueError(f"Манифест превышает max_objects_per_cycle: {key}")
    for ref in manifest["objects"]:
        entity_key = _apply_ref(s3, config, checkpoint, mappings, ref, mapping_version, store, deadline)
        if manifest["mode"] == "local_snapshot":
            if entity_key in present:
                raise ValueError("Snapshot содержит повторную сущность")
            present.add(entity_key)
    if manifest["mode"] == "local_snapshot":
        from ..reconciliation import reconcile_snapshot
        allowed = frozenset(manifest["complete_entity_types"]) & config.reconcile_entity_types
        try:
            reconcile_snapshot(config, checkpoint, store, present, manifest["run_id"], manifest,
                               mapping_version, entity_types=allowed)
        except Exception:
            raise RuntimeError("Snapshot reconciliation failed; payload omitted") from None
    _ensure_deadline(deadline)
    checkpoint.finish_v2_run(manifest["run_id"], key, manifest.get("created_at"),
                              manifest["mode"], manifest["epoch_id"], digest,
                              mapping_version)


def _reproject_active(s3, config, checkpoint, mappings, mapping_version, store, deadline=None):
    for row in checkpoint.active_entities():
        if row["mapping_version"] == mapping_version:
            continue
        key = row["source_key"]
        if not isinstance(key, str):
            raise RuntimeError("Перепроекция заблокирована: у активной сущности нет source key")
        parts = key[len(config.events_prefix):].split("/") if key.startswith(config.events_prefix) else []
        if len(parts) != 3 or parts[0] not in SUPPORTED_ENTITIES:
            raise RuntimeError("Перепроекция заблокирована: source key вне v2 events prefix")
        event_id = parts[2][:-5] if parts[2].endswith(".json") else ""
        ref = {"entity": parts[0], "event_id": event_id,
               "graph_id": f"urn:bitrix24:graph:{config.portal_id}:{event_id}",
               "key": key, "sha256": row["sha256"]}
        try:
            _apply_ref(s3, config, checkpoint, mappings, ref, mapping_version, store, deadline)
        except ClientError as exc:
            code = str((exc.response.get("Error") or {}).get("Code", ""))
            if code in {"NoSuchKey", "404", "NotFound"} and parts[0] == "activity":
                LOG.error("Перепроекция остановлена: исходное activity событие вышло из retention")
            raise RuntimeError("Перепроекция остановлена: исходный v2 объект недоступен") from exc


def run_cycle_v2(s3, config, checkpoint, mappings, store, state=None, deadline=None):
    # One immutable state read defines this cycle's confirmed upper bound.
    deadline = deadline if deadline is not None else time.monotonic() + config.max_cycle_read_seconds
    state, state_sha = read_contract_state(s3, config, deadline) if state is None else state
    _ensure_deadline(deadline)
    previous_revision = int(checkpoint.get_meta("observed_state_revision") or 0)
    previous_state_sha = checkpoint.get_meta("observed_state_sha256")
    if state["revision"] < previous_revision:
        raise RuntimeError("state.json revision отстаёт от v2 checkpoint")
    if (state["revision"] == previous_revision and previous_revision > 0
            and previous_state_sha and state_sha != previous_state_sha):
        raise RuntimeError("state.json изменился без увеличения revision")
    mapping_version = mapping_fingerprint(config, mappings)
    base, base_sha = _base_snapshot(s3, config, state, deadline)
    if len(base["objects"]) > config.max_objects_per_cycle:
        raise ValueError(f"Snapshot превышает max_objects_per_cycle={config.max_objects_per_cycle}")

    saved_epoch = checkpoint.get_meta("epoch_id")
    saved_base_sha = checkpoint.get_meta("base_manifest_sha256")
    if saved_epoch == state["epoch_id"] and saved_base_sha not in (None, base_sha):
        raise RuntimeError("Checkpoint и base snapshot SHA конфликтуют внутри одной epoch")
    if saved_epoch != state["epoch_id"] or saved_base_sha != base_sha:
        _apply_manifest(s3, config, checkpoint, mappings,
                        (state["base_manifest_key"], base, base_sha), mapping_version, store, deadline)
    elif not config.fast_resume:
        for ref in base["objects"]:
            _verify_ref(s3, config, ref, deadline)

    chain = _chain(s3, config, state, checkpoint, deadline)
    total_objects = len(base["objects"]) + sum(len(manifest["objects"]) for _, manifest, _ in chain)
    if total_objects > config.max_objects_per_cycle:
        raise ValueError(f"Snapshot + цепочка превышают max_objects_per_cycle={config.max_objects_per_cycle}")
    for key, manifest, digest in chain:
        # The checkpoint may already include this exact chain prefix.
        if (checkpoint.get_meta("applied_tip_run_id") == manifest["run_id"]
                and checkpoint.get_meta("applied_tip_manifest_sha256") == digest
                and checkpoint.get_meta("epoch_id") == state["epoch_id"]):
            if not config.fast_resume:
                for ref in manifest["objects"]:
                    _verify_ref(s3, config, ref, deadline)
            continue
        _apply_manifest(s3, config, checkpoint, mappings, (key, manifest, digest),
                        mapping_version, store, deadline)
    _reproject_active(s3, config, checkpoint, mappings, mapping_version, store, deadline)
    _ensure_deadline(deadline)
    checkpoint.set_meta("observed_state_revision", str(state["revision"]))
    checkpoint.set_meta("observed_state_sha256", state_sha)
    checkpoint.set_meta("mapping_version", mapping_version)
    return state
