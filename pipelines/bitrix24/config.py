from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
import yaml
from .identity import SUPPORTED_ENTITIES

@dataclass(frozen=True)
class Config:
    bucket: str
    portal_id: str
    endpoint_url: str
    region: str
    access_key: str
    secret_key: str
    backend: str
    store_url: str
    store_username: str | None
    store_password: str | None
    store_timeout: int
    graphdb_repository: str | None
    graph_namespace: str
    registry_path: Path
    schemas_root: Path
    checkpoint_path: Path
    fast_resume: bool
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
    backend = raw.get("backend")
    if backend not in {"fuseki", "graphdb"}:
        raise ValueError(f"Неизвестный backend: {backend!r}; выберите fuseki или graphdb")
    store = raw.get(backend) or {}
    if not isinstance(store, dict):
        raise ValueError(f"{backend} должен быть объектом")
    mapping = raw.get("mapping") or {}
    checkpoint = raw.get("checkpoint") or {}
    resume_mode = checkpoint.get("resume_mode", "strict")
    if resume_mode not in {"strict", "fast"}:
        raise ValueError("checkpoint.resume_mode должен быть strict или fast")
    missing = [key for key in ("bucket", "portal_id", "access_key", "secret_key") if not s3.get(key)]
    if missing:
        raise ValueError(f"Не заданы параметры s3: {', '.join(missing)}")
    if not store.get("url"):
        raise ValueError(f"Не задан {backend}.url")
    if backend == "graphdb" and not store.get("repository"):
        raise ValueError("Не задан graphdb.repository")
    if backend == "graphdb" and ("/" in str(store["repository"]) or not str(store["repository"]).strip()):
        raise ValueError("graphdb.repository должен быть именем репозитория")
    from urllib.parse import urlparse
    parsed = urlparse(str(store["url"]))
    if parsed.scheme not in {"http", "https"} or not parsed.netloc:
        raise ValueError(f"Некорректный {backend}.url")
    timeout = int(store.get("timeout_seconds", 90))
    if timeout < 1:
        raise ValueError("timeout_seconds должен быть положительным")
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
        secret_key=s3["secret_key"], backend=backend, store_url=str(store["url"]).rstrip("/"),
        store_username=store.get("username") or None, store_password=store.get("password") or None,
        store_timeout=timeout, graphdb_repository=store.get("repository") if backend == "graphdb" else None,
        graph_namespace=store.get("graph_namespace", "urn:bitrix24:entity"),
        registry_path=resolve(mapping["registry_path"]),
        schemas_root=resolve(mapping["schemas_root"]),
        checkpoint_path=resolve(checkpoint.get("path", "state.sqlite3")),
        fast_resume=resume_mode == "fast",
        poll_interval=max(1, int(raw.get("poll_interval_seconds", 60))),
        reconcile_entity_types=reconcile,
    )
