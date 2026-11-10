#!/usr/bin/env python3
"""Copy a stopped loader checkpoint and bind the copy to GraphDB."""
from __future__ import annotations

import argparse
import sqlite3
from pathlib import Path


def meta(db: sqlite3.Connection, key: str) -> str | None:
    row = db.execute("SELECT value FROM meta WHERE key = ?", (key,)).fetchone()
    return row[0] if row else None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--target-identity", required=True)
    parser.add_argument("--expected-last-run-id", required=True)
    args = parser.parse_args()

    source = args.source.resolve(strict=True)
    destination = args.destination.resolve()
    if source == destination:
        parser.error("source and destination must be different")
    if destination.exists():
        parser.error(f"destination already exists: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)

    src = sqlite3.connect(source)
    dst = sqlite3.connect(destination)
    try:
        src.backup(dst)
        if meta(dst, "store_backend") != "fuseki":
            parser.error("source checkpoint is not bound to Fuseki")
        actual_run_id = meta(dst, "last_run_id") or ""
        if actual_run_id != args.expected_last_run_id:
            parser.error(
                f"checkpoint last_run_id is {actual_run_id!r}, "
                f"expected {args.expected_last_run_id!r}"
            )
        result = dst.execute("PRAGMA integrity_check").fetchone()[0]
        if result != "ok":
            parser.error(f"checkpoint integrity check failed: {result}")
        with dst:
            dst.execute(
                "UPDATE meta SET value = 'graphdb' WHERE key = 'store_backend'"
            )
            dst.execute(
                "UPDATE meta SET value = ? WHERE key = 'store_identity'",
                (args.target_identity,),
            )
        active = dst.execute("SELECT COUNT(*) FROM entities WHERE deleted = 0").fetchone()[0]
        final_integrity = dst.execute("PRAGMA integrity_check").fetchone()[0]
        if final_integrity != "ok":
            parser.error(f"migrated checkpoint integrity check failed: {final_integrity}")
        print(f"Checkpoint copied and rebound; last_run_id={actual_run_id}; active_entities={active}")
    finally:
        dst.close()
        src.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
