from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import logging
import os
import shutil
import sqlite3
import stat
import sys
import tarfile
import time
import uuid
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import urlsplit, urlunsplit

import yaml

from ..bitrix24.config import load_config
from ..bitrix24.cli import make_store
from .graphdb import GraphDBRecovery
from .fuseki import FusekiBackup

LOG = logging.getLogger("backup")


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def assign_package_owner(directory: Path) -> None:
    """Return a root-run backup package to the host user configured by Compose."""
    uid = os.environ.get("BACKUP_OUTPUT_UID")
    gid = os.environ.get("BACKUP_OUTPUT_GID")
    if uid is None and gid is None:
        return
    if uid is None or gid is None:
        raise ValueError("BACKUP_OUTPUT_UID и BACKUP_OUTPUT_GID нужно задавать вместе")
    uid, gid = int(uid), int(gid)
    if os.geteuid() != 0:
        if os.geteuid() != uid or os.getegid() != gid:
            raise PermissionError("Backup-пользователь не совпадает с BACKUP_OUTPUT_UID/GID")
        return
    for path in (directory, *directory.rglob("*")):
        os.chown(path, uid, gid)


def preserve_checkpoint_owner(source: Path, restored: Path) -> None:
    """Keep the active checkpoint's owner and permissions when replacing it."""
    active = source.stat()
    current = restored.stat()
    if (current.st_uid, current.st_gid) != (active.st_uid, active.st_gid):
        if os.geteuid() != 0:
            raise PermissionError("Не удалось сохранить владельца активного checkpoint")
        os.chown(restored, active.st_uid, active.st_gid)
    os.chmod(restored, stat.S_IMODE(active.st_mode))


def safe_url(value: str) -> str:
    parsed = urlsplit(value)
    host = parsed.hostname or ""
    if parsed.port:
        host += f":{parsed.port}"
    return urlunsplit((parsed.scheme, host, parsed.path, "", ""))


def read_yaml_config(path: Path) -> tuple[object, dict]:
    raw = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    if raw.get("backend") not in {"graphdb", "fuseki"}:
        raise ValueError("Backup-команда принимает конфигурацию backend: graphdb или fuseki")
    graphdb = raw.get("graphdb") or {}
    backup_user = graphdb.get("backup_username") or graphdb.get("username")
    backup_password = graphdb.get("backup_password") or graphdb.get("password")
    if bool(backup_user) != bool(backup_password):
        raise ValueError("Для GraphDB Recovery API задайте оба значения username и password или оставьте оба пустыми")
    return load_config(path), raw


def make_recovery(config, raw: dict) -> GraphDBRecovery:
    db = raw["graphdb"]
    return GraphDBRecovery(config.store_url, config.graphdb_repository,
                           db.get("backup_username") or config.store_username,
                           db.get("backup_password") or config.store_password,
                           config.store_timeout)


def make_fuseki(config, raw: dict) -> FusekiBackup:
    fuseki = raw.get("fuseki") or {}
    backup_directory = fuseki.get("backup_directory")
    if not backup_directory:
        raise ValueError("Задайте fuseki.backup_directory — путь Fuseki BASE/backups, доступный backup-контейнеру")
    dataset = config.store_url.rstrip("/").rsplit("/", 1)[-1]
    return FusekiBackup(config.store_url, dataset, config.store_username, config.store_password,
                        Path(backup_directory), config.store_timeout,
                        int(fuseki.get("backup_timeout_seconds", 1800)))


def store_adapter(config, raw: dict):
    return make_recovery(config, raw) if config.backend == "graphdb" else make_fuseki(config, raw)


class LoaderBarrier:
    def __init__(self, checkpoint_path: Path, timeout: int):
        self.state_dir = checkpoint_path.parent
        self.timeout = timeout
        self.request_path = self.state_dir / "backup.request.json"
        self.status_path = self.state_dir / "status.json"
        self.lock_path = checkpoint_path.with_suffix(checkpoint_path.suffix + ".lock")
        self.backup_id = f"backup-{datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')}-{uuid.uuid4().hex[:8]}"
        self.active = False
        self.offline_lock = None

    def _status(self) -> dict:
        try:
            return json.loads(self.status_path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            return {}

    def acquire(self, offline: bool = False) -> dict:
        self.state_dir.mkdir(parents=True, exist_ok=True)
        if offline:
            self.offline_lock = self.lock_path.open("a+")
            try:
                fcntl.flock(self.offline_lock.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError as exc:
                self.offline_lock.close()
                self.offline_lock = None
                raise RuntimeError("Загрузчик всё ещё владеет checkpoint; offline backup/restore отменён") from exc
            status = self._status()
            if status.get("state") not in {"waiting", "idle", "paused"}:
                fcntl.flock(self.offline_lock.fileno(), fcntl.LOCK_UN)
                self.offline_lock.close()
                self.offline_lock = None
                raise RuntimeError("Offline backup/restore разрешён только после остановки загрузчика на границе цикла")
            self.active = True
            return status
        if self.request_path.exists():
            raise RuntimeError(f"Уже существует запрос backup barrier: {self.request_path}")
        temp = self.request_path.with_suffix(".json.tmp")
        temp.write_text(json.dumps({"backup_id": self.backup_id, "requested_at": utc_now()}) + "\n",
                         encoding="utf-8")
        os.replace(temp, self.request_path)
        self.active = True
        deadline = time.monotonic() + self.timeout
        while time.monotonic() < deadline:
            status = self._status()
            maintenance = status.get("maintenance") or {}
            if status.get("state") == "paused" and maintenance.get("backup_id") == self.backup_id:
                LOG.info("Загрузчик приостановлен после цикла run=%s", status.get("last_run_id"))
                return status
            time.sleep(1)
        self.release()
        raise TimeoutError("Загрузчик не подтвердил backup barrier; проверьте версию контейнера и журнал")

    def release(self, wait_for_resume: bool = True) -> None:
        if not self.active:
            return
        if self.offline_lock is not None:
            fcntl.flock(self.offline_lock.fileno(), fcntl.LOCK_UN)
            self.offline_lock.close()
            self.offline_lock = None
            self.active = False
            return
        try:
            request = json.loads(self.request_path.read_text(encoding="utf-8"))
            if request.get("backup_id") == self.backup_id:
                self.request_path.unlink()
        except FileNotFoundError:
            pass
        self.active = False
        if wait_for_resume:
            deadline = time.monotonic() + min(self.timeout, 60)
            while time.monotonic() < deadline:
                status = self._status()
                maintenance = status.get("maintenance") or {}
                if maintenance.get("state") == "resumed" and maintenance.get("backup_id") == self.backup_id:
                    return
                time.sleep(1)
            LOG.warning("Запрос снят, но статус возобновления загрузчика пока не появился")


def sqlite_backup(source: Path, destination: Path) -> dict:
    if not source.is_file():
        raise FileNotFoundError(f"Checkpoint не найден: {source}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.unlink(missing_ok=True)
    src = sqlite3.connect(f"{source.resolve().as_uri()}?mode=ro", uri=True)
    dst = sqlite3.connect(destination)
    try:
        src.backup(dst)
        journal_mode = dst.execute("PRAGMA journal_mode=DELETE").fetchone()[0]
        if journal_mode.lower() != "delete":
            raise RuntimeError(f"Не удалось сделать checkpoint переносимым: journal_mode={journal_mode}")
        result = dst.execute("PRAGMA integrity_check").fetchone()[0]
        if result != "ok":
            raise RuntimeError(f"SQLite checkpoint integrity_check: {result}")
        meta = dict(dst.execute("SELECT key,value FROM meta"))
        entities = dst.execute("SELECT COUNT(*), SUM(CASE WHEN deleted=0 THEN 1 ELSE 0 END) FROM entities").fetchone()
        run_count = dst.execute("SELECT COUNT(*) FROM runs").fetchone()[0]
        mappings = [row[0] for row in dst.execute(
            "SELECT DISTINCT mapping_version FROM entities WHERE mapping_version IS NOT NULL ORDER BY mapping_version")]
        return {"integrity": result, "meta": meta,
                "entity_rows": entities[0], "active_entities": entities[1] or 0,
                "run_rows": run_count, "mapping_versions": mappings}
    finally:
        dst.close()
        src.close()


def inspect_checkpoint(path: Path) -> dict:
    connection = sqlite3.connect(f"{path.resolve().as_uri()}?mode=ro", uri=True)
    try:
        result = connection.execute("PRAGMA integrity_check").fetchone()[0]
        if result != "ok":
            raise RuntimeError(f"SQLite checkpoint integrity_check: {result}")
        meta = dict(connection.execute("SELECT key,value FROM meta"))
        entities = connection.execute(
            "SELECT COUNT(*), SUM(CASE WHEN deleted=0 THEN 1 ELSE 0 END) FROM entities").fetchone()
        runs = connection.execute("SELECT COUNT(*) FROM runs").fetchone()[0]
        return {"integrity": result, "meta": meta,
                "entity_rows": entities[0], "active_entities": entities[1] or 0,
                "run_rows": runs}
    finally:
        connection.close()


def package_file(directory: Path, relative_path: str) -> Path:
    root = directory.resolve()
    candidate = directory / relative_path
    if candidate.is_symlink():
        raise ValueError(f"Артефакт backup не должен быть символической ссылкой: {relative_path}")
    path = candidate.resolve()
    if path != root and root not in path.parents:
        raise ValueError(f"Путь артефакта выходит за пределы backup-пакета: {relative_path}")
    return path


def verify_package(directory: Path) -> tuple[dict, Path, Path]:
    manifest_path = directory / "manifest.json"
    if not manifest_path.is_file():
        raise FileNotFoundError(f"manifest.json не найден: {directory}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("format") != "semantic-bi-backup/v1":
        raise ValueError("Неподдерживаемая версия backup manifest")
    artifacts = manifest.get("artifacts") or {}
    store_path = package_file(directory, artifacts["store"]["path"])
    checkpoint_path = package_file(directory, artifacts["checkpoint"]["path"])
    for name, path in (("store", store_path), ("checkpoint", checkpoint_path)):
        expected = artifacts[name]["sha256"]
        if (not path.is_file() or path.stat().st_size != artifacts[name].get("bytes")
                or sha256(path) != expected):
            raise ValueError(f"Проверка SHA-256 не прошла для {name}: {path}")
    backend = manifest.get("store", {}).get("backend")
    archive_format = artifacts["store"].get("format")
    if (backend, archive_format) == ("graphdb", "graphdb-native-tar"):
        GraphDBRecovery.verify_backup(store_path)
    elif (backend, archive_format) == ("fuseki", "fuseki-nquads-gzip"):
        FusekiBackup.verify_backup(store_path)
    else:
        raise ValueError(f"Неизвестное соответствие backend/формата артефакта: {backend!r}/{archive_format!r}")
    checkpoint = inspect_checkpoint(checkpoint_path)
    if checkpoint["meta"].get("store_backend") != backend:
        raise ValueError(f"Checkpoint backup не привязан к {backend}")
    if safe_url(checkpoint["meta"].get("store_identity", "")) != manifest.get("store", {}).get("identity"):
        raise ValueError("Checkpoint identity не совпадает с manifest backup")
    if checkpoint["meta"].get("last_run_id") != manifest.get("sync", {}).get("last_run_id"):
        raise ValueError("Checkpoint cursor не совпадает с manifest backup")
    return manifest, store_path, checkpoint_path


def create_backup(config, raw: dict, output: Path, barrier_timeout: int, offline: bool = False) -> Path:
    adapter = store_adapter(config, raw)
    if config.backend == "graphdb":
        adapter.ensure_admin_access()
        server_info = {"version": adapter.version()}
        store_name = config.graphdb_repository
    else:
        server_info = adapter.ensure_access()
        store_name = adapter.dataset
    barrier = LoaderBarrier(config.checkpoint_path, barrier_timeout)
    status = barrier.acquire(offline=offline)
    staging = None
    try:
        output.mkdir(parents=True, exist_ok=True)
        final_dir = output / barrier.backup_id
        staging = output / f".{barrier.backup_id}.partial"
        if final_dir.exists() or staging.exists():
            raise FileExistsError(f"Каталог backup уже существует: {final_dir}")
        staging.mkdir(mode=0o700)
        store_dir = staging / "store"
        store_dir.mkdir()
        rdf_stats = adapter.dataset_stats()
        if config.backend == "graphdb":
            store_archive = store_dir / "graphdb-repository.tar"
            adapter.create_backup(store_archive, barrier.backup_id)
            store_format = "graphdb-native-tar"
        else:
            store_archive = store_dir / "fuseki-dataset.nq.gz"
            adapter.create_backup(store_archive)
            store_format = "fuseki-nquads-gzip"
        stats_after = adapter.dataset_stats()
        if stats_after != rdf_stats:
            raise RuntimeError(f"RDF dataset изменился во время backup: до={rdf_stats}, после={stats_after}")
        engine_version = server_info.get("version")
        checkpoint_file = staging / "checkpoint.sqlite"
        checkpoint_info = sqlite_backup(config.checkpoint_path, checkpoint_file)
        expected_identity = make_store(config).endpoint
        if checkpoint_info["meta"].get("store_identity") != expected_identity:
            raise RuntimeError(f"Checkpoint identity не совпадает с endpoint текущего {config.backend}")
        for key in ("last_run_id", "base_manifest_key"):
            status_value = status.get(key)
            checkpoint_value = checkpoint_info["meta"].get(key)
            if status_value and checkpoint_value != status_value:
                raise RuntimeError(f"Status/checkpoint не совпали по {key}; backup-пакет не создан")
        files = {
            "store": {"path": store_archive.relative_to(staging).as_posix(), "bytes": store_archive.stat().st_size,
                      "sha256": sha256(store_archive), "format": store_format},
            "checkpoint": {"path": "checkpoint.sqlite", "bytes": checkpoint_file.stat().st_size,
                           "sha256": sha256(checkpoint_file), "format": "sqlite"},
        }
        store_manifest = {"backend": config.backend, "target": store_name,
                          "endpoint": safe_url(config.store_url),
                          "identity": safe_url(expected_identity),
                          "engine_version": engine_version}
        if config.backend == "graphdb":
            store_manifest["repository"] = store_name
        else:
            store_manifest["dataset"] = store_name
        manifest = {
            "format": "semantic-bi-backup/v1", "backup_id": barrier.backup_id,
            "created_at": utc_now(), "state": "complete",
            "store": store_manifest,
            "sync": {"last_run_id": status.get("last_run_id"),
                     "base_manifest_key": status.get("base_manifest_key"),
                     "mode": status.get("mode"),
                     "mapping_versions": checkpoint_info["mapping_versions"]},
            "rdf": rdf_stats,
            "checkpoint": {"integrity": checkpoint_info["integrity"],
                           "entity_rows": checkpoint_info["entity_rows"],
                           "active_entities": checkpoint_info["active_entities"],
                           "run_rows": checkpoint_info["run_rows"],
                           "store_backend": checkpoint_info["meta"].get("store_backend"),
                           "store_identity": safe_url(checkpoint_info["meta"].get("store_identity", ""))},
            "artifacts": files,
        }
        (staging / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
                                                 encoding="utf-8")
        checksums = [f"{files[key]['sha256']}  {files[key]['path']}" for key in ("store", "checkpoint")]
        (staging / "SHA256SUMS").write_text("\n".join(checksums) + "\n", encoding="ascii")
        assign_package_owner(staging)
        os.replace(staging, final_dir)
        LOG.info("Backup %s завершён: %s", config.backend, final_dir)
        return final_dir
    except Exception:
        if staging is not None:
            shutil.rmtree(staging, ignore_errors=True)
        raise
    finally:
        barrier.release()


def restore_backup(config, raw: dict, directory: Path, confirmation: str,
                   barrier_timeout: int, offline: bool = False) -> None:
    manifest, archive, checkpoint_file = verify_package(directory)
    backend = config.backend
    adapter = store_adapter(config, raw)
    target_name = config.graphdb_repository if backend == "graphdb" else adapter.dataset
    if confirmation != target_name:
        option = "--confirm-repository" if backend == "graphdb" else "--confirm-dataset"
        raise ValueError(f"Для восстановления передайте {option} с точным именем цели")
    if manifest.get("store", {}).get("backend") != backend:
        raise ValueError(f"Backup предназначен для backend {manifest.get('store', {}).get('backend')!r}")
    saved_target = manifest.get("store", {}).get("target") or manifest.get("store", {}).get("repository")
    if saved_target != target_name:
        raise ValueError("Имя цели в backup не совпадает с текущей конфигурацией")
    expected_identity = make_store(config).endpoint
    if manifest.get("store", {}).get("identity") != safe_url(expected_identity):
        raise ValueError(f"Backup привязан к другому {backend} endpoint; перепривязка не поддерживается")
    if backend == "graphdb":
        adapter.ensure_admin_access()
    else:
        adapter.ensure_access()
    barrier = LoaderBarrier(config.checkpoint_path, barrier_timeout)
    barrier.acquire(offline=offline)
    restore_started = False
    checkpoint_temp = config.checkpoint_path.with_suffix(".restore.tmp")
    rollback = config.checkpoint_path.parent / f".{directory.name}.fuseki-rollback.nq.gz"
    try:
        checkpoint_temp.unlink(missing_ok=True)
        checkpoint_info = sqlite_backup(checkpoint_file, checkpoint_temp)
        if checkpoint_info["meta"].get("store_identity") != expected_identity:
            raise ValueError("Checkpoint в backup привязан к другому endpoint")
        if checkpoint_info["meta"].get("store_backend") != backend:
            raise ValueError(f"Checkpoint в backup не привязан к {backend}")
        if checkpoint_info["meta"].get("last_run_id") != manifest.get("sync", {}).get("last_run_id"):
            raise ValueError("Позиция checkpoint не совпадает с manifest backup")

        if backend == "fuseki":
            # A pre-restore image allows rollback if the transactional dataset
            # replacement fails or its post-restore statistics do not match.
            rollback_stats = adapter.dataset_stats()
            adapter.create_backup(rollback)
        restore_started = True
        try:
            adapter.restore_backup(archive)
            actual_stats = adapter.dataset_stats()
            if actual_stats != manifest.get("rdf"):
                raise RuntimeError(f"Статистика RDF после восстановления не совпала с backup: {actual_stats}")
        except Exception:
            if backend == "fuseki" and rollback.is_file():
                try:
                    LOG.exception("Восстановление Fuseki не прошло проверку; возвращаю исходный dataset")
                    adapter.restore_backup(rollback)
                    if adapter.dataset_stats() != rollback_stats:
                        raise RuntimeError("Статистика dataset после отката Fuseki не совпала с исходной")
                    rollback.unlink(missing_ok=True)
                    restore_started = False
                except Exception:
                    LOG.critical("Откат Fuseki также не удался; загрузчик оставлен на паузе")
                    raise
            raise

        preserve_checkpoint_owner(config.checkpoint_path, checkpoint_temp)
        os.replace(checkpoint_temp, config.checkpoint_path)
        Path(str(config.checkpoint_path) + "-wal").unlink(missing_ok=True)
        Path(str(config.checkpoint_path) + "-shm").unlink(missing_ok=True)
        rollback.unlink(missing_ok=True)
        LOG.info("%s и парный checkpoint восстановлены из %s", backend, directory)
    except Exception:
        checkpoint_temp.unlink(missing_ok=True)
        if restore_started:
            LOG.critical("Восстановление начато; загрузчик оставлен на паузе. Проверьте хранилище и checkpoint перед ручным resume.")
        else:
            barrier.release()
        raise
    else:
        barrier.release()


def main(argv=None) -> None:
    parser = argparse.ArgumentParser(description="Coordinated RDF store + loader checkpoint backup/restore")
    parser.add_argument("--config", type=Path, default=Path("deploy/graphdb/config.yaml"))
    parser.add_argument("--pause-timeout", type=int, default=600)
    sub = parser.add_subparsers(dest="command", required=True)
    create = sub.add_parser("create", help="Создать согласованный RDF store backup и checkpoint")
    create.add_argument("--output", type=Path, default=Path("/backups"))
    create.add_argument("--offline", action="store_true", help="Загрузчик заранее остановлен; занять checkpoint lock")
    verify = sub.add_parser("verify", help="Проверить целостность готового backup-пакета")
    verify.add_argument("package", type=Path)
    restore = sub.add_parser("restore", help="Восстановить RDF-хранилище и checkpoint из одного пакета")
    restore.add_argument("package", type=Path)
    confirmation = restore.add_mutually_exclusive_group(required=True)
    confirmation.add_argument("--confirm-repository", help="Точное имя GraphDB repository")
    confirmation.add_argument("--confirm-dataset", help="Точное имя Fuseki dataset")
    restore.add_argument("--offline", action="store_true", help="Загрузчик заранее остановлен; занять checkpoint lock")
    sub.add_parser("resume", help="Снять зависшую паузу загрузчика после ручной проверки")
    args = parser.parse_args(argv)
    logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")

    if args.command == "resume":
        resume_config = load_config(args.config.resolve())
        request = resume_config.checkpoint_path.parent / "backup.request.json"
        request.unlink(missing_ok=True)
        LOG.warning("Запрос паузы удалён; используйте resume только после проверки согласованности RDF и checkpoint")
        return
    if args.command == "verify":
        manifest, _, _ = verify_package(args.package.resolve())
        print(json.dumps({"valid": True, "backup_id": manifest["backup_id"],
                          "backend": manifest["store"]["backend"],
                          "target": manifest["store"].get("target") or manifest["store"].get("repository"),
                          "created_at": manifest["created_at"]}, ensure_ascii=False))
        return

    config_path = args.config.resolve()
    config, raw = read_yaml_config(config_path)
    if args.command == "create":
        print(create_backup(config, raw, args.output.resolve(), args.pause_timeout, args.offline))
    elif args.command == "restore":
        restore_backup(config, raw, args.package.resolve(), args.confirm_repository or args.confirm_dataset,
                       args.pause_timeout, args.offline)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        LOG.error("Операция backup/restore завершилась ошибкой: %s", exc)
        sys.exit(1)
