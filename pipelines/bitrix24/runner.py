from __future__ import annotations
import logging
import hashlib
import json
import re
from datetime import timedelta
from botocore.exceptions import ClientError
from ..core.checkpoint import Checkpoint
from ..core.graph_store import GraphStore
from ..core.runner import apply_graph_operation
from .reconciliation import reconcile_snapshot
from .config import Config
from .identity import SUPPORTED_ENTITIES, entity_graph_uri
from .mapper import MappingEntry, mapping_fingerprint, transform_event
from .source import get_object_bytes, get_json, list_run_manifest_keys, validate_event, validate_manifest, parse_time, run_id_time, is_stale, not_newer_than, read_exporter_state, read_base_snapshot
LOG = logging.getLogger("rdf_sync")


def checkpoint_matches_ref(config: Config, checkpoint: Checkpoint,
                           ref: dict[str, Any], mapping_version: str) -> str | None:
    """Return the entity key when this exact event was already applied.

    Fast resume trusts the manifest SHA and the SHA stored after the prior
    full-body verification; it deliberately avoids fetching the event body.
    """
    key = ref.get("key")
    if not isinstance(key, str) or not key.startswith(config.events_prefix):
        return None
    parts = key[len(config.events_prefix):].split("/")
    if len(parts) != 3:
        return None
    entity_type, identity, filename = parts
    if entity_type not in SUPPORTED_ENTITIES or not filename.endswith(".json"):
        return None
    event_id = filename[:-5]
    digest = ref.get("sha256")
    if (not event_id or event_id != ref.get("event_id")
            or ref.get("entity") != entity_type
            or ref.get("graph_id") != f"urn:bitrix24:graph:{config.portal_id}:{event_id}"
            or not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest)):
        return None
    entity_key = f"{config.portal_id}/{entity_type}/{identity}"
    graph_uri = entity_graph_uri(config.graph_namespace, entity_key)
    previous = checkpoint.get_entity(entity_key)
    if (previous and previous["source_key"] == key
            and previous["event_id"] == event_id
            and previous["sha256"] == digest
            and previous["graph_uri"] == graph_uri
            and previous["mapping_version"] == mapping_version):
        return entity_key
    return None

def apply_event(s3, config: Config, checkpoint: Checkpoint, mappings: list[MappingEntry],
                ref: dict[str, Any], mapping_version: str,
                authoritative_snapshot: bool = False,
                baseline_modified_at: str | None = None, *, store: GraphStore) -> str:
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
        apply_graph_operation(store, "delete", graph_uri,
            lambda: checkpoint.save_entity(entity_key, graph_uri, event["event_id"], modified, True,
                                           key, actual_sha, mapping_version))
    else:
        rdfxml = transform_event(config, mappings, event, relative_key)
        apply_graph_operation(store, "replace", graph_uri,
            lambda: checkpoint.save_entity(entity_key, graph_uri, event["event_id"], modified, False,
                                           key, actual_sha, mapping_version), rdfxml)
    LOG.info("Применено событие %s в граф %s", event["event_id"], graph_uri)
    return entity_key


def process_manifest(s3, config: Config, checkpoint: Checkpoint,
                     mappings: list[MappingEntry], manifest_key: str, manifest: dict[str, Any],
                     mode: str, mapping_version: str,
                     baseline_modified_at: str | None = None, *, store: GraphStore) -> set[str]:
    run_id = manifest["run_id"]
    refs = validate_manifest(manifest, run_id, mode)
    present: set[str] = set()
    fast_skipped = 0
    for ref in refs:
        entity_key = None
        if config.fast_resume:
            entity_key = checkpoint_matches_ref(config, checkpoint, ref, mapping_version)
            if entity_key is not None:
                fast_skipped += 1
        if entity_key is None:
            entity_key = apply_event(
                s3, config, checkpoint, mappings, ref, mapping_version,
                authoritative_snapshot=(mode == "local_snapshot"),
                baseline_modified_at=baseline_modified_at, store=store,
            )
        if mode == "local_snapshot" and entity_key in present:
            raise ValueError(f"Полный снимок содержит несколько версий {entity_key}")
        present.add(entity_key)
    if mode == "local_snapshot":
        reconcile_snapshot(config, checkpoint, store, present, run_id, manifest, mapping_version)
    checkpoint.save_run(run_id, manifest_key, manifest.get("created_at"), mode)
    if fast_skipped:
        LOG.info("Быстрое восстановление: пропущено скачивание %d из %d уже применённых событий",
                 fast_skipped, len(refs))
    return present


def maybe_apply_snapshot(s3, config: Config, checkpoint: Checkpoint,
                         mappings: list[MappingEntry], exporter_state: dict[str, Any],
                         mapping_version: str, store: GraphStore) -> tuple[str, bool, set[str]]:
    key, manifest, _refs = read_base_snapshot(s3, config, exporter_state)
    catchup_to = manifest.get("catchup_to")
    if not parse_time(catchup_to):
        raise ValueError(f"В полном снимке {key} некорректное поле catchup_to")
    changed = checkpoint.get_meta("base_manifest_key") != key
    present: set[str] = set()
    if changed:
        LOG.info("Применяется базовый снимок %s", key)
        present = process_manifest(s3, config, checkpoint, mappings, key, manifest,
                                   "local_snapshot", mapping_version, store=store)
        checkpoint.set_meta("base_manifest_key", key)
        checkpoint.set_meta("base_catchup_to", catchup_to)
        checkpoint.set_meta("last_run_id", "")
    return catchup_to, changed, present


def run_incrementals(s3, config: Config, checkpoint: Checkpoint,
                     mappings: list[MappingEntry], exporter_state: dict[str, Any],
                     catchup_to: str, mapping_version: str, store: GraphStore) -> None:
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
                         "incremental", mapping_version, catchup_to, store=store)
        checkpoint.set_meta("last_run_id", run_id)
        LOG.info("Применён инкрементальный запуск %s", run_id)
        if run_id == latest_run_id:
            break


def run_cycle(s3, config: Config, checkpoint: Checkpoint, mappings: list[MappingEntry], store: GraphStore) -> None:
    exporter_state = read_exporter_state(s3, config)
    current_mapping_version = mapping_fingerprint(config, mappings)
    catchup_to, _snapshot_changed, snapshot_entities = maybe_apply_snapshot(
        s3, config, checkpoint, mappings, exporter_state, current_mapping_version, store
    )
    old_mapping_version = checkpoint.get_meta("mapping_version")
    if old_mapping_version and old_mapping_version != current_mapping_version:
        LOG.info("Версия CCCR mappings изменилась; пересобираются текущие графы")
        active_rows = checkpoint.active_entities()
        total_entities = len(active_rows)
        reprojected = 0
        checked = 0
        unavailable_activities = 0
        for index, row in enumerate(active_rows, 1):
            checked = index
            if (row["entity_key"] in snapshot_entities
                    or row["mapping_version"] == current_mapping_version):
                if index % 250 == 0 or index == total_entities:
                    LOG.info("Перепроекция графов: %d/%d проверено, %d пересобрано, %d старых activity без источника",
                             checked, total_entities, reprojected, unavailable_activities)
                continue
            entity_type = row["entity_key"].split("/", 2)[1]
            ref = {
                "entity": entity_type,
                "event_id": row["event_id"], "graph_id": f"urn:bitrix24:graph:{config.portal_id}:{row['event_id']}",
                "key": row["source_key"], "sha256": row["sha256"],
            }
            try:
                apply_event(s3, config, checkpoint, mappings, ref, current_mapping_version, store=store)
            except ClientError as exc:
                error_code = (exc.response.get("Error") or {}).get("Code")
                if entity_type != "activity" or error_code not in {"NoSuchKey", "404", "NotFound"}:
                    raise
                # Activities are retained in Fuseki beyond the exporter's 90-day S3
                # window; an expired source event cannot be used to rebuild the graph.
                unavailable_activities += 1
            else:
                reprojected += 1
            if index % 250 == 0 or index == total_entities:
                LOG.info("Перепроекция графов: %d/%d проверено, %d пересобрано, %d старых activity без источника",
                         checked, total_entities, reprojected, unavailable_activities)
        if unavailable_activities:
            LOG.warning(
                "%d старых графов activity оставлены без перепроекции: исходные события уже отсутствуют в S3",
                unavailable_activities,
            )
    run_incrementals(s3, config, checkpoint, mappings, exporter_state,
                     catchup_to, current_mapping_version, store)
    checkpoint.set_meta("mapping_version", current_mapping_version)
