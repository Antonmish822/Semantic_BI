from __future__ import annotations
import fcntl
import sqlite3
from pathlib import Path

class Checkpoint:
    def __init__(self, path: Path):
        self.path = path
        path.parent.mkdir(parents=True, exist_ok=True)
        self.lock_file = path.with_suffix(path.suffix + ".lock").open("a+")
        try:
            fcntl.flock(self.lock_file.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as exc:
            self.lock_file.close()
            raise RuntimeError(f"Другой загрузчик уже использует checkpoint {path}") from exc
        self.db = sqlite3.connect(path)
        self.db.row_factory = sqlite3.Row
        if self.db.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='meta'").fetchone():
            row = self.db.execute("SELECT value FROM meta WHERE key='checkpoint_version'").fetchone()
            if row and row["value"] not in {"1", "2"}:
                self.db.close()
                fcntl.flock(self.lock_file.fileno(), fcntl.LOCK_UN)
                self.lock_file.close()
                raise RuntimeError(f"Неизвестная версия checkpoint: {row['value']}")
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
        run_columns = {row[1] for row in self.db.execute("PRAGMA table_info(runs)")}
        for name in ("epoch_id", "manifest_sha256"):
            if name not in run_columns:
                self.db.execute(f"ALTER TABLE runs ADD COLUMN {name} TEXT")
        self.db.commit()

    def bind_store(self, backend: str, identity: str) -> None:
        """Bind legacy Fuseki state or a fresh database to one RDF target."""
        version = self.get_meta("checkpoint_version")
        if version not in (None, "1"):
            raise RuntimeError(f"Неизвестная версия checkpoint: {version}")
        bound_backend = self.get_meta("store_backend")
        bound_identity = self.get_meta("store_identity")
        if bound_backend is None:
            has_history = (self.db.execute("SELECT 1 FROM entities LIMIT 1").fetchone()
                           or self.db.execute("SELECT 1 FROM runs LIMIT 1").fetchone()
                           or self.get_meta("base_manifest_key"))
            if has_history and backend != "fuseki":
                raise RuntimeError("Существующий checkpoint без привязки принадлежит Fuseki; для GraphDB нужен отдельный файл")
        elif (bound_backend, bound_identity) != (backend, identity):
            raise RuntimeError(f"Checkpoint привязан к {bound_backend} {bound_identity}, выбран {backend} {identity}")
        with self.db:
            for key, value in (("checkpoint_version", "1"), ("store_backend", backend), ("store_identity", identity)):
                self.db.execute("INSERT INTO meta(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value", (key, value))

    def bind_store_v2(self, backend: str, identity: str) -> None:
        """Bind a v2 checkpoint without adopting or downgrading legacy state."""
        version = self.get_meta("checkpoint_version")
        protocol = self.get_meta("contract_version")
        if version == "1" or protocol == "1":
            raise RuntimeError("Checkpoint v1 нельзя автоматически использовать для контракта 2; требуется отдельный checkpoint и snapshot")
        if version not in (None, "2") or protocol not in (None, "2"):
            raise RuntimeError("Checkpoint привязан к несовместимой версии протокола")
        bound_backend = self.get_meta("store_backend")
        bound_identity = self.get_meta("store_identity")
        if bound_backend is None:
            has_history = (self.db.execute("SELECT 1 FROM entities LIMIT 1").fetchone()
                           or self.db.execute("SELECT 1 FROM runs LIMIT 1").fetchone())
            if has_history:
                raise RuntimeError("Checkpoint без версии протокола содержит историю; требуется отдельный файл v2")
        elif (bound_backend, bound_identity) != (backend, identity):
            raise RuntimeError(f"Checkpoint привязан к {bound_backend} {bound_identity}, выбран {backend} {identity}")
        with self.db:
            for key, value in (("checkpoint_version", "2"), ("contract_version", "2"),
                               ("store_backend", backend), ("store_identity", identity)):
                self.db.execute("INSERT INTO meta(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value", (key, value))

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

    def save_v2_run(self, run_id: str, manifest_key: str, created_at: str | None,
                    mode: str, epoch_id: str, manifest_sha256: str) -> None:
        with self.db:
            old = self.db.execute("SELECT epoch_id,manifest_sha256 FROM runs WHERE run_id=?", (run_id,)).fetchone()
            if old and (old["epoch_id"] not in (None, epoch_id)
                        or old["manifest_sha256"] not in (None, manifest_sha256)):
                raise RuntimeError(f"Конфликт повторного run {run_id}: epoch/SHA отличаются")
            self.db.execute("""INSERT INTO runs(run_id,manifest_key,created_at,mode,epoch_id,manifest_sha256)
                VALUES(?,?,?,?,?,?) ON CONFLICT(run_id) DO UPDATE SET
                manifest_key=excluded.manifest_key, created_at=excluded.created_at,
                mode=excluded.mode, epoch_id=excluded.epoch_id, manifest_sha256=excluded.manifest_sha256""",
                (run_id, manifest_key, created_at, mode, epoch_id, manifest_sha256))

    def finish_v2_run(self, run_id: str, manifest_key: str, created_at: str | None,
                      mode: str, epoch_id: str, manifest_sha256: str,
                      mapping_version: str) -> None:
        """Atomically record run identity and its protocol cursor."""
        with self.db:
            old = self.db.execute("SELECT epoch_id,manifest_sha256 FROM runs WHERE run_id=?", (run_id,)).fetchone()
            if old and (old["epoch_id"] not in (None, epoch_id)
                        or old["manifest_sha256"] not in (None, manifest_sha256)):
                raise RuntimeError(f"Конфликт повторного run {run_id}: epoch/SHA отличаются")
            self.db.execute("""INSERT INTO runs(run_id,manifest_key,created_at,mode,epoch_id,manifest_sha256)
                VALUES(?,?,?,?,?,?) ON CONFLICT(run_id) DO UPDATE SET
                manifest_key=excluded.manifest_key, created_at=excluded.created_at,
                mode=excluded.mode, epoch_id=excluded.epoch_id, manifest_sha256=excluded.manifest_sha256""",
                (run_id, manifest_key, created_at, mode, epoch_id, manifest_sha256))
            values = {"epoch_id": epoch_id, "mapping_version": mapping_version}
            if mode == "local_snapshot":
                values.update({"base_manifest_key": manifest_key, "base_manifest_sha256": manifest_sha256,
                               "applied_tip_run_id": "", "applied_tip_manifest_sha256": "",
                               "observed_state_revision": "0"})
            elif mode == "incremental":
                values.update({"applied_tip_run_id": run_id,
                               "applied_tip_manifest_sha256": manifest_sha256})
            for key, value in values.items():
                self.db.execute("INSERT INTO meta(key,value) VALUES(?,?) ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                                (key, value))

    def entities_missing_from(self, present: set[str], entity_types: frozenset[str]):
        rows = self.db.execute("SELECT * FROM entities WHERE deleted=0").fetchall()
        for row in rows:
            parts = row["entity_key"].split("/", 2)
            if len(parts) == 3 and parts[1] in entity_types and row["entity_key"] not in present:
                yield row

    def close(self) -> None:
        if self.db is not None:
            self.db.close()
        fcntl.flock(self.lock_file.fileno(), fcntl.LOCK_UN)
        self.lock_file.close()

    def suspend(self) -> None:
        """Close SQLite while retaining the process-wide checkpoint lock."""
        if self.db is None:
            return
        self.db.execute("PRAGMA wal_checkpoint(TRUNCATE)")
        self.db.close()
        self.db = None

    def resume(self) -> None:
        """Reopen the database after maintenance without releasing its lock."""
        if self.db is not None:
            return
        self.db = sqlite3.connect(self.path)
        self.db.row_factory = sqlite3.Row
        self.db.execute("PRAGMA journal_mode=WAL")
