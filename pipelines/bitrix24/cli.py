"""Bitrix24 S3 to RDF named graph loader."""
import argparse
import logging
import json
import os
import time
from datetime import datetime, timezone
from pathlib import Path
import boto3
from botocore.exceptions import ClientError
from botocore.config import Config as BotoConfig

from .config import load_config
from .mapper import load_mappings
from .v2.runner import read_contract_state, run_cycle_v2, apply_backfill_manifest
from ..core.checkpoint import Checkpoint
from ..stores.fuseki import FusekiStore
from ..stores.graphdb import GraphDBStore


def make_s3(config):
    return boto3.client(
        "s3", endpoint_url=config.endpoint_url, region_name=config.region,
        aws_access_key_id=config.access_key, aws_secret_access_key=config.secret_key,
        config=BotoConfig(connect_timeout=min(10, config.s3_read_timeout_seconds),
                          read_timeout=config.s3_read_timeout_seconds,
                          retries={"max_attempts": 3, "mode": "standard"}),
    )


def make_store(config):
    args = (config.store_url, config.store_username, config.store_password, config.store_timeout)
    if config.backend == "fuseki":
        return FusekiStore(*args)
    if config.backend == "graphdb":
        return GraphDBStore(config.store_url, config.graphdb_repository,
                            config.store_username, config.store_password, config.store_timeout)
    raise ValueError(f"Неизвестный backend: {config.backend}")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Bitrix24 S3 manifests → named RDF graphs")
    parser.add_argument("--config", type=Path, default=Path("deploy/loader/config.yaml"))
    parser.add_argument("--once", action="store_true", help="Выполнить один цикл")
    parser.add_argument("--apply-backfill-key", help="Явно применить lead-only backfill manifest")
    parser.add_argument("--apply-backfill-sha256", help="SHA-256 указанного backfill manifest")
    parser.add_argument("--log-level", default="INFO")
    args = parser.parse_args(argv)
    logging.basicConfig(level=getattr(logging, args.log_level.upper(), logging.INFO),
                        format="%(asctime)s [%(levelname)s] %(message)s")
    config = load_config(args.config.resolve())
    logging.getLogger("rdf_sync").info(
        "Режим восстановления: %s", "fast" if config.fast_resume else "strict"
    )
    store = make_store(config)
    mappings = load_mappings(config)
    s3 = make_s3(config)
    checkpoint = Checkpoint(config.checkpoint_path)
    status_path = config.checkpoint_path.parent / "status.json"
    backup_request_path = config.checkpoint_path.parent / "backup.request.json"
    status_snapshot = {"base_manifest_key": None}

    def write_status(**extra):
        if checkpoint is not None and checkpoint.db is not None:
            status_snapshot["base_manifest_key"] = checkpoint.get_meta("base_manifest_key")
            status_snapshot.update({
                "contract_version": 2,
                "epoch_id": checkpoint.get_meta("epoch_id"),
                "base_manifest_sha256": checkpoint.get_meta("base_manifest_sha256"),
                "applied_tip_run_id": checkpoint.get_meta("applied_tip_run_id") or None,
                "applied_tip_manifest_sha256": checkpoint.get_meta("applied_tip_manifest_sha256") or None,
                "observed_state_revision": int(checkpoint.get_meta("observed_state_revision") or 0),
            })
        payload = {"updated_at": datetime.now(timezone.utc).isoformat(),
                   "mode": "once" if args.once else "continuous",
                   **status_snapshot, **extra}
        if "maintenance" not in extra:
            payload.pop("maintenance", None)
        temp = status_path.with_suffix(".json.tmp")
        temp.write_text(json.dumps(payload, ensure_ascii=False) + "\n", encoding="utf-8")
        os.replace(temp, status_path)

    def pause_for_backup():
        nonlocal checkpoint
        try:
            request = json.loads(backup_request_path.read_text(encoding="utf-8"))
        except FileNotFoundError:
            return
        except (OSError, ValueError) as exc:
            logging.getLogger("rdf_sync").error("Некорректный запрос backup barrier: %s", exc)
            return
        backup_id = request.get("backup_id")
        if not isinstance(backup_id, str) or not backup_id:
            logging.getLogger("rdf_sync").error("Запрос backup barrier не содержит backup_id")
            return

        # Finish the current cycle before closing SQLite. The backup process can
        # then capture one stable RDF/checkpoint pair and atomically replace it
        # during a restore without a live SQLite connection.
        checkpoint.suspend()
        write_status(state="paused", phase="backup_barrier", error=None,
                     maintenance={"state": "paused", "backup_id": backup_id})
        logging.getLogger("rdf_sync").info("Загрузчик приостановлен на backup barrier %s", backup_id)
        while True:
            time.sleep(1)
            try:
                current = json.loads(backup_request_path.read_text(encoding="utf-8"))
            except FileNotFoundError:
                break
            except (OSError, ValueError):
                continue
            if current.get("backup_id") != backup_id:
                break

        checkpoint.resume()
        checkpoint.bind_store_v2(config.backend, identity)
        write_status(state="waiting", phase="cycle", error=None,
                     maintenance={"state": "resumed", "backup_id": backup_id})
        logging.getLogger("rdf_sync").info("Загрузчик возобновлён после backup barrier %s", backup_id)

    identity = store.endpoint
    try:
        if args.apply_backfill_key or args.apply_backfill_sha256:
            if not args.apply_backfill_key or not args.apply_backfill_sha256:
                raise ValueError("Для backfill нужны и --apply-backfill-key, и --apply-backfill-sha256")
            exporter_state, _state_sha = read_contract_state(s3, config)
            checkpoint.bind_store_v2(config.backend, identity)
            result = apply_backfill_manifest(s3, config, checkpoint, mappings, store,
                                             args.apply_backfill_key, args.apply_backfill_sha256)
            logging.getLogger("rdf_sync").info("Backfill applied: %s", result)
            return
        while True:
            pause_for_backup()
            cycle_started = time.monotonic()
            current_phase = "reading_state"
            current_progress = {"phase": current_phase}
            write_status(state="running", phase=current_phase, error=None)
            logging.getLogger("rdf_sync").info("Cycle started: mode=%s; reading state.json from S3",
                                                "once" if args.once else "continuous")
            try:
                cycle_deadline = time.monotonic() + config.max_cycle_read_seconds
                exporter_state, state_sha = read_contract_state(s3, config, cycle_deadline)
                checkpoint.bind_store_v2(config.backend, identity)
                current_phase = "contract_v2"
                write_status(state="running", phase="contract_v2", error=None,
                             contract_version=2, observed_state_revision=exporter_state["revision"])

                def report_progress(**progress):
                    nonlocal current_phase, current_progress
                    current_phase = progress.get("phase", current_phase)
                    details = dict(progress)
                    progress_state = details.pop("state", "running")
                    current_progress = details
                    write_status(state=progress_state, error=None, **details)

                run_cycle_v2(s3, config, checkpoint, mappings, store,
                             state=(exporter_state, state_sha), deadline=cycle_deadline,
                             progress=report_progress)
            except Exception as exc:
                state_label = "blocked" if isinstance(exc, (ValueError, KeyError, ClientError)) else "error"
                write_status(state=state_label, phase=current_phase, error="cycle_failed",
                             **{k: v for k, v in current_progress.items() if k != "phase"})
                logging.getLogger("rdf_sync").exception(
                    "Cycle failed: phase=%s; checkpoint сохранён для повтора", current_phase)
                if args.once:
                    raise
            else:
                elapsed = round(time.monotonic() - cycle_started, 1)
                write_status(state="idle" if args.once else "waiting", phase="cycle", error=None)
                logging.getLogger("rdf_sync").info("Cycle finished: duration_seconds=%s", elapsed)
            pause_for_backup()
            if args.once:
                return
            logging.getLogger("rdf_sync").info(
                "Waiting for next cycle: poll_interval_seconds=%s", config.poll_interval)
            for _ in range(config.poll_interval):
                pause_for_backup()
                time.sleep(1)
    finally:
        if checkpoint is not None:
            checkpoint.close()


if __name__ == "__main__":
    main()
