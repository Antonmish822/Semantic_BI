"""Prepare a separate v2 SQLite checkpoint from an offline v1 copy."""
from __future__ import annotations

import argparse
import fcntl
import os
import sqlite3
from pathlib import Path


def prepare(source: Path, destination: Path, expected_backend: str, expected_identity: str) -> dict:
    source, destination = source.resolve(), destination.resolve()
    if source == destination or destination.exists():
        raise ValueError("Destination must be a new path distinct from source")
    if not source.is_file():
        raise FileNotFoundError(source)
    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_name(destination.name + ".prepare.tmp")
    if temporary.exists():
        raise FileExistsError(temporary)
    lock_path = source.with_suffix(source.suffix + ".lock")
    lock = lock_path.open("a+")
    try:
        try:
            fcntl.flock(lock.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as exc:
            raise RuntimeError("Source checkpoint is in use; stop the loader before preparation") from exc
        src = sqlite3.connect(f"{source.as_uri()}?mode=ro", uri=True)
        dst = sqlite3.connect(temporary)
        try:
            src.row_factory = sqlite3.Row
            meta = dict(src.execute("SELECT key,value FROM meta"))
            if meta.get("checkpoint_version") != "1" or meta.get("contract_version") not in (None, "1"):
                raise ValueError("Source checkpoint is not a v1 checkpoint")
            if meta.get("store_backend") != expected_backend or meta.get("store_identity") != expected_identity:
                raise ValueError("Source checkpoint RDF backend/identity does not match the requested target")
            result = src.execute("PRAGMA integrity_check").fetchone()[0]
            if result != "ok":
                raise RuntimeError(f"Source checkpoint integrity check failed: {result}")
            src.backup(dst)
            with dst:
                dst.execute("DELETE FROM runs")
                for key in ("last_run_id", "base_manifest_key", "base_catchup_to",
                            "epoch_id", "base_manifest_sha256", "applied_tip_run_id",
                            "applied_tip_manifest_sha256", "observed_state_revision",
                            "observed_state_sha256", "transition_prepared_at"):
                    dst.execute("DELETE FROM meta WHERE key=?", (key,))
                for key, value in (("checkpoint_version", "2"), ("contract_version", "2"),
                                   ("store_backend", expected_backend), ("store_identity", expected_identity),
                                   ("transition_prepared", "v1_entities_preserved_snapshot_required")):
                    dst.execute("INSERT INTO meta(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                                (key, value))
            dst.execute("PRAGMA wal_checkpoint(TRUNCATE)")
            verified = dst.execute("PRAGMA integrity_check").fetchone()[0]
            if verified != "ok":
                raise RuntimeError(f"Prepared checkpoint integrity check failed: {verified}")
            entity_count = dst.execute("SELECT COUNT(*) FROM entities").fetchone()[0]
            active_count = dst.execute("SELECT COUNT(*) FROM entities WHERE deleted=0").fetchone()[0]
            run_count = dst.execute("SELECT COUNT(*) FROM runs").fetchone()[0]
            if run_count:
                raise RuntimeError("Prepared checkpoint still contains v1 run cursor rows")
        except Exception:
            dst.close()
            src.close()
            temporary.unlink(missing_ok=True)
            raise
        else:
            dst.close()
            src.close()
        os.replace(temporary, destination)
        return {"destination": str(destination), "backend": expected_backend,
                "identity": expected_identity, "entity_rows": entity_count,
                "active_entities": active_count, "run_rows": run_count,
                "checkpoint_version": 2, "snapshot_required": True}
    finally:
        fcntl.flock(lock.fileno(), fcntl.LOCK_UN)
        lock.close()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="offline v1 checkpoint copy")
    parser.add_argument("destination", type=Path, help="new v2 checkpoint path")
    parser.add_argument("--backend", choices=("fuseki", "graphdb"), required=True)
    parser.add_argument("--identity", required=True, help="exact RDF Store endpoint identity")
    args = parser.parse_args(argv)
    result = prepare(args.source, args.destination, args.backend, args.identity)
    for key, value in result.items():
        print(f"{key}={value}")


if __name__ == "__main__":
    main()
