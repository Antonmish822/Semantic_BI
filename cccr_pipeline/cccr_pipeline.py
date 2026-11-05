#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CCCR pipeline: S3 -> (regex -> манифест(ы) CCCR) -> RDF -> Jena Fuseki.

Скрипт раз в config.poll_interval_seconds опрашивает S3-хранилище, находит
новые/изменившиеся файлы (XML, CSV), подбирает для каждого из них все
подходящие манифесты CCCR (по регулярным выражениям из реестра манифестов),
прогоняет файл через движок CCCR (библиотека cccr_py из приложенного
архива) и заливает получившийся RDF-граф в Jena Fuseki как отдельный
именованный граф "<имя_файла_без_расширения>_<имя_манифеста>".

Требования:
    - Собранный и установленный модуль cccr_py (см. README архива:
      cccr-engine/pyproject.toml -> `pip install .`). Модуль должен быть
      импортируемым в этом окружении Python (тот же python3.x, под который
      он собран).
    - config.yaml рядом со скриптом (см. config.example.yaml).
    - В S3-бакете, в папке манифестов (s3.manifests_prefix), должен лежать
      файл-реестр манифестов (s3.registry_key), см. описание формата в
      README.md рядом со скриптом.

Запуск:
    python3 cccr_pipeline.py
Остановка:
    Ctrl+C / SIGTERM (текущий цикл опроса будет завершён корректно).
"""

from __future__ import annotations

import argparse
import json
import logging
import re
import signal
import sys
import time
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

import boto3
import requests
import yaml

try:
    import cccr_py as cccr
except ImportError as exc:  # pragma: no cover - диагностическое сообщение
    sys.stderr.write(
        "Не удалось импортировать модуль 'cccr_py'.\n"
        "Соберите и установите библиотеку CCCR из приложенного архива "
        "(cccr-engine/pyproject.toml -> `pip install .`, требуются "
        "системные зависимости: SaxonC-HE, Redland/raptor2/rasqal/librdf, "
        "см. README.md в архиве) и убедитесь, что она видна из текущего "
        "интерпретатора Python.\n"
        f"Исходная ошибка импорта: {exc}\n"
    )
    raise SystemExit(1) from exc


# --------------------------------------------------------------------------
# Пути и константы
# --------------------------------------------------------------------------

SCRIPT_DIR = Path(__file__).resolve().parent
CONFIG_PATH = SCRIPT_DIR / "config.yaml"
STATE_PATH = SCRIPT_DIR / "state.json"
MANIFEST_CACHE_DIR = SCRIPT_DIR / ".manifests_cache"

LOG = logging.getLogger("cccr_pipeline")


# --------------------------------------------------------------------------
# Конфигурация
# --------------------------------------------------------------------------

@dataclass
class S3Config:
    bucket: str
    key_id: str
    secret_key: str
    endpoint_url: str = "https://storage.yandexcloud.net"
    region: str = "ru-central1"
    input_prefix: str = ""
    manifests_prefix: str = "manifests/"
    registry_key: str = "manifests/registry.json"
    service_account: str = ""  # информационно, для авторизации не требуется


@dataclass
class FusekiConfig:
    url: str
    username: Optional[str] = None
    password: Optional[str] = None
    graph_namespace: str = "urn:cccr:"


@dataclass
class AppConfig:
    s3: S3Config
    fuseki: FusekiConfig
    poll_interval: int = 60
    allowed_extensions: tuple = (".xml", ".csv", ".json")


def load_config(path: Path) -> AppConfig:
    if not path.exists():
        raise SystemExit(
            f"Не найден файл конфигурации: {path}\n"
            "Создайте config.yaml на основе config.example.yaml."
        )
    raw = yaml.safe_load(path.read_text(encoding="utf-8")) or {}

    s3_raw = raw.get("s3") or {}
    fuseki_raw = raw.get("fuseki") or {}

    missing = [k for k in ("bucket", "key_id", "secret_key") if not s3_raw.get(k)]
    if missing:
        raise SystemExit(f"В config.yaml в разделе 's3' не заданы поля: {', '.join(missing)}")
    if not fuseki_raw.get("url"):
        raise SystemExit("В config.yaml в разделе 'fuseki' не задано поле 'url'")

    manifests_prefix = s3_raw.get("manifests_prefix", "manifests/")
    if manifests_prefix and not manifests_prefix.endswith("/"):
        manifests_prefix += "/"

    s3_cfg = S3Config(
        bucket=s3_raw["bucket"],
        key_id=s3_raw["key_id"],
        secret_key=s3_raw["secret_key"],
        endpoint_url=s3_raw.get("endpoint_url", "https://storage.yandexcloud.net"),
        region=s3_raw.get("region", "ru-central1"),
        input_prefix=s3_raw.get("input_prefix", ""),
        manifests_prefix=manifests_prefix,
        registry_key=s3_raw.get("registry_key", manifests_prefix + "registry.json"),
        service_account=s3_raw.get("service_account", ""),
    )
    fuseki_cfg = FusekiConfig(
        url=fuseki_raw["url"],
        username=fuseki_raw.get("username"),
        password=fuseki_raw.get("password"),
        graph_namespace=fuseki_raw.get("graph_namespace", "urn:cccr:"),
    )
    poll_interval = int(raw.get("poll_interval_seconds", 60))
    return AppConfig(s3=s3_cfg, fuseki=fuseki_cfg, poll_interval=poll_interval)


# --------------------------------------------------------------------------
# Реестр манифестов (regex -> манифест)
# --------------------------------------------------------------------------

_FLAG_MAP = {"i": re.IGNORECASE, "m": re.MULTILINE, "s": re.DOTALL}


@dataclass
class ManifestEntry:
    manifest_key: str      # ключ в S3 (полный, включая manifests_prefix)
    local_path: Path       # локальный кэшированный путь к JSON-схеме
    pattern: re.Pattern
    match_mode: str = "search"  # search | match | fullmatch

    @property
    def name(self) -> str:
        """Имя манифеста, используется в имени выходного графа."""
        return Path(self.manifest_key).stem

    def matches(self, filename: str) -> bool:
        fn = getattr(self.pattern, self.match_mode)
        return fn(filename) is not None


def _load_registry_data(local_registry_path: Path) -> list:
    text = local_registry_path.read_text(encoding="utf-8")
    if local_registry_path.suffix.lower() in (".yaml", ".yml"):
        data = yaml.safe_load(text)
    else:
        data = json.loads(text)
    if not isinstance(data, list):
        raise ValueError(
            f"Реестр манифестов {local_registry_path} должен быть JSON/YAML-списком объектов"
        )
    return data


def _build_manifest_entries(registry_data: list, manifests_prefix: str, cache_dir: Path) -> list:
    entries = []
    for item in registry_data:
        try:
            manifest_rel = item.get("manifest") or item.get("schema")
            pattern_str = item["pattern"]
        except KeyError as exc:
            raise ValueError(f"Некорректная запись реестра манифестов: {item}") from exc
        if not manifest_rel:
            raise ValueError(f"Некорректная запись реестра манифестов: {item}")

        flags = 0
        for ch in item.get("flags", ""):
            flags |= _FLAG_MAP.get(ch, 0)

        match_mode = item.get("match", "search")
        if match_mode not in ("search", "match", "fullmatch"):
            raise ValueError(f"Некорректный режим 'match' в реестре: {match_mode!r}")

        entries.append(
            ManifestEntry(
                manifest_key=manifests_prefix + manifest_rel,
                local_path=cache_dir / manifest_rel,
                pattern=re.compile(pattern_str, flags),
                match_mode=match_mode,
            )
        )
    return entries


def sync_manifests(s3, cfg: S3Config, cache_dir: Path) -> list:
    """Синхронизирует папку манифестов из S3 в локальный кэш и возвращает
    список ManifestEntry, собранный из реестра манифестов."""
    cache_dir.mkdir(parents=True, exist_ok=True)
    etag_cache_path = cache_dir / ".etags.json"
    etag_cache = {}
    if etag_cache_path.exists():
        try:
            etag_cache = json.loads(etag_cache_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            etag_cache = {}

    seen_keys = set()
    paginator = s3.get_paginator("list_objects_v2")
    for page in paginator.paginate(Bucket=cfg.bucket, Prefix=cfg.manifests_prefix):
        for obj in page.get("Contents", []):
            key = obj["Key"]
            if key.endswith("/"):
                continue
            seen_keys.add(key)
            rel = key[len(cfg.manifests_prefix):] if key.startswith(cfg.manifests_prefix) else key
            local_file = cache_dir / rel
            etag = obj["ETag"].strip('"')
            if etag_cache.get(key) != etag:
                local_file.parent.mkdir(parents=True, exist_ok=True)
                s3.download_file(cfg.bucket, key, str(local_file))
                etag_cache[key] = etag
                LOG.info("Обновлён манифест в локальном кэше: %s", key)

    # удаляем из кэша то, что пропало из S3
    for old_key in list(etag_cache.keys()):
        if old_key not in seen_keys:
            rel = old_key[len(cfg.manifests_prefix):] if old_key.startswith(cfg.manifests_prefix) else old_key
            local_file = cache_dir / rel
            if local_file.exists():
                local_file.unlink()
            del etag_cache[old_key]

    etag_cache_path.write_text(json.dumps(etag_cache, ensure_ascii=False, indent=2), encoding="utf-8")

    registry_key = cfg.registry_key
    registry_rel = registry_key[len(cfg.manifests_prefix):] if registry_key.startswith(cfg.manifests_prefix) else registry_key
    registry_local = cache_dir / registry_rel
    if not registry_local.exists():
        raise FileNotFoundError(
            f"Реестр манифестов не найден: S3-ключ '{registry_key}' "
            f"(ожидался локально в {registry_local}). Проверьте s3.registry_key в config.yaml."
        )

    registry_data = _load_registry_data(registry_local)
    return _build_manifest_entries(registry_data, cfg.manifests_prefix, cache_dir)


# --------------------------------------------------------------------------
# Обработка входных файлов
# --------------------------------------------------------------------------

def list_input_objects(s3, cfg: S3Config) -> list:
    results = []
    paginator = s3.get_paginator("list_objects_v2")
    for page in paginator.paginate(Bucket=cfg.bucket, Prefix=cfg.input_prefix):
        for obj in page.get("Contents", []):
            key = obj["Key"]
            if key.endswith("/"):
                continue
            if key.startswith(cfg.manifests_prefix):
                continue
            if key == cfg.registry_key:
                continue
            results.append(obj)
    return results


def _append_value(parent: ET.Element, name: str, value) -> None:
    element = ET.SubElement(parent, name)
    if value is None:
        element.set("nil", "true")
    elif isinstance(value, dict):
        for key, child in value.items():
            _append_value(element, str(key), child)
    elif isinstance(value, list):
        for child in value:
            _append_value(element, "item", child)
    elif isinstance(value, bool):
        element.text = "true" if value else "false"
    else:
        element.text = str(value)


def _json_to_xml_text(text: str, source: str) -> str:
    payload = json.loads(text)
    if not isinstance(payload, dict):
        raise ValueError(f"Ожидался JSON-объект: {source}")
    if isinstance(payload.get("payload"), dict):
        payload = payload["payload"]
    root = ET.Element("json")
    record = ET.SubElement(root, "object")
    for field, value in payload.items():
        _append_value(record, field, value)
    try:
        ET.indent(root, space="  ")
    except AttributeError:
        pass
    return ET.tostring(root, encoding="utf-8", xml_declaration=True).decode("utf-8")


def apply_manifest(entry: ManifestEntry, ext: str, text: str, filename: str,
                    fuseki_cfg: FusekiConfig) -> bool:
    """Применяет один манифест к содержимому файла и заливает результат в Fuseki."""
    ctx = cccr.Context()

    try:
        schema_text = entry.local_path.read_text(encoding="utf-8")
    except OSError as exc:
        LOG.error("Не удалось прочитать манифест %s: %s", entry.local_path, exc)
        return False

    schema = ctx.load_schema(schema_text, include_base=str(entry.local_path.parent))
    if schema is None:
        LOG.error("Ошибка загрузки манифеста %s: %s", entry.manifest_key, ctx.last_error)
        return False

    if ext == ".xml":
        source = ctx.load_xml(text)
    elif ext == ".csv":
        source = ctx.load_csv(text)
    elif ext == ".json":
        try:
            xml_text = _json_to_xml_text(text, filename)
        except (json.JSONDecodeError, ValueError) as exc:
            LOG.error("Ошибка чтения JSON %s: %s", filename, exc)
            return False
        source = ctx.load_xml(xml_text)
    else:
        LOG.error("Неподдерживаемый тип файла %s для манифеста %s", filename, entry.manifest_key)
        return False

    if source is None:
        LOG.error("Ошибка загрузки данных из %s: %s", filename, ctx.last_error)
        return False

    graph = ctx.run(schema, source)
    if graph is None:
        LOG.error(
            "Ошибка применения манифеста %s к файлу %s: %s",
            entry.manifest_key, filename, ctx.last_error,
        )
        return False

    rdfxml = graph.to_rdfxml()
    if rdfxml is None:
        LOG.error(
            "Ошибка сериализации RDF (%s / %s): %s",
            filename, entry.manifest_key, ctx.last_error,
        )
        return False

    out_name = f"{Path(filename).stem}_{entry.name}"
    return push_to_fuseki(fuseki_cfg, out_name, rdfxml)


def push_to_fuseki(cfg: FusekiConfig, name: str, rdfxml: str) -> bool:
    """Заливает RDF/XML в Fuseki как именованный граф <graph_namespace><name>
    через Graph Store HTTP Protocol (PUT .../data?graph=...)."""
    graph_uri = cfg.graph_namespace + name
    data_endpoint = cfg.url.rstrip("/") + "/data"
    auth = (cfg.username, cfg.password) if cfg.username else None
    try:
        resp = requests.put(
            data_endpoint,
            params={"graph": graph_uri},
            data=rdfxml.encode("utf-8"),
            headers={"Content-Type": "application/rdf+xml; charset=utf-8"},
            auth=auth,
            timeout=60,
        )
    except requests.RequestException as exc:
        LOG.error("Не удалось связаться с Fuseki (%s): %s", data_endpoint, exc)
        return False

    if resp.status_code in (200, 201, 204):
        LOG.info("Загружено в Fuseki: граф '%s' (%s)", graph_uri, name)
        return True

    LOG.error(
        "Fuseki отклонил загрузку графа '%s': HTTP %s: %s",
        graph_uri, resp.status_code, resp.text[:500],
    )
    return False


def process_object(s3, fuseki_cfg: FusekiConfig, cfg: S3Config, obj: dict,
                    entries: list, state: dict, allowed_extensions: tuple) -> None:
    key = obj["Key"]
    etag = obj["ETag"].strip('"')
    filename = Path(key).name
    ext = Path(key).suffix.lower()

    if ext not in allowed_extensions:
        return

    relative = key[len(cfg.input_prefix):] if key.startswith(cfg.input_prefix) else key
    matched = [e for e in entries if e.matches(relative)]
    matched_keys = {e.manifest_key for e in matched}

    file_state = state["files"].get(key, {})
    prev_etag = file_state.get("etag")
    done = set(file_state.get("done_manifests", [])) if prev_etag == etag else set()

    if not matched:
        LOG.warning(
            "Не найден манифест для файла '%s' — файл не обработан, "
            "попробую снова на следующем опросе", key,
        )
        state["files"][key] = {
            "etag": etag,
            "done_manifests": sorted(done),
            "pending": True,
        }
        return

    # Файл уже полностью обработан всеми подходящими манифестами -> пропускаем
    if prev_etag == etag and matched_keys.issubset(done):
        return

    LOG.info(
        "Файл '%s': подобрано манифестов — %d (%s)",
        key, len(matched), ", ".join(e.manifest_key for e in matched),
    )

    obj_bytes = s3.get_object(Bucket=cfg.bucket, Key=key)["Body"].read()
    text = obj_bytes.decode("utf-8-sig")

    all_ok = True
    for entry in matched:
        if entry.manifest_key in done:
            continue
        try:
            ok = apply_manifest(entry, ext, text, filename, fuseki_cfg)
        except Exception:
            LOG.exception(
                "Необработанная ошибка при применении манифеста %s к файлу %s",
                entry.manifest_key, key,
            )
            ok = False
        if ok:
            done.add(entry.manifest_key)
        else:
            all_ok = False

    state["files"][key] = {
        "etag": etag,
        "done_manifests": sorted(done),
        "pending": not all_ok,
    }
    if all_ok:
        LOG.info("Файл '%s' полностью обработан (%d манифест(ов))", key, len(matched))
    else:
        LOG.warning("Файл '%s' обработан частично — повторим оставшееся на следующем опросе", key)


# --------------------------------------------------------------------------
# Состояние
# --------------------------------------------------------------------------

def load_state(path: Path) -> dict:
    if path.exists():
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
            data.setdefault("files", {})
            return data
        except json.JSONDecodeError:
            LOG.warning("Файл состояния %s повреждён, начинаю с чистого состояния", path)
    return {"files": {}}


def save_state(path: Path, state: dict) -> None:
    tmp = path.with_suffix(".tmp")
    tmp.write_text(json.dumps(state, ensure_ascii=False, indent=2), encoding="utf-8")
    tmp.replace(path)


# --------------------------------------------------------------------------
# S3-клиент и основной цикл
# --------------------------------------------------------------------------

def make_s3_client(cfg: S3Config):
    session = boto3.session.Session()
    return session.client(
        "s3",
        endpoint_url=cfg.endpoint_url,
        region_name=cfg.region,
        aws_access_key_id=cfg.key_id,
        aws_secret_access_key=cfg.secret_key,
    )


_STOP = False


def _handle_stop(signum, _frame):
    global _STOP
    LOG.info("Получен сигнал остановки (%s) — завершу текущий цикл и выйду", signum)
    _STOP = True


def run_cycle(s3, cfg: AppConfig, state: dict) -> None:
    entries = sync_manifests(s3, cfg.s3, MANIFEST_CACHE_DIR)
    objects = list_input_objects(s3, cfg.s3)
    for obj in objects:
        if _STOP:
            break
        process_object(s3, cfg.fuseki, cfg.s3, obj, entries, state, cfg.allowed_extensions)
    save_state(STATE_PATH, state)


def _parse_args():
    parser = argparse.ArgumentParser(
        description="CCCR S3 → Fuseki pipeline"
    )
    parser.add_argument(
        "--config",
        default=str(CONFIG_PATH),
        help="Путь к config.yaml (по умолчанию рядом со скриптом)",
    )
    parser.add_argument(
        "--once",
        action="store_true",
        help="Выполнить один цикл и завершиться",
    )
    parser.add_argument(
        "--log-level",
        default="INFO",
        help="Уровень логирования (DEBUG, INFO, WARNING, ERROR)",
    )
    return parser.parse_args()


def main() -> None:
    args = _parse_args()

    level = getattr(logging, str(args.log_level).upper(), logging.INFO)
    logging.basicConfig(
        level=level,
        format="%(asctime)s [%(levelname)s] %(message)s",
    )
    signal.signal(signal.SIGINT, _handle_stop)
    signal.signal(signal.SIGTERM, _handle_stop)

    config_path = Path(args.config).resolve() if args.config else CONFIG_PATH
    cfg = load_config(config_path)
    s3 = make_s3_client(cfg.s3)
    state = load_state(STATE_PATH)

    LOG.info(
        "Старт конвейера CCCR. Бакет='%s', input_prefix='%s', опрос каждые %s сек.",
        cfg.s3.bucket, cfg.s3.input_prefix, cfg.poll_interval,
    )

    if args.once:
        run_cycle(s3, cfg, state)
        LOG.info("Однократный цикл завершён — выхожу")
        return

    while not _STOP:
        cycle_start = time.monotonic()
        try:
            run_cycle(s3, cfg, state)
        except Exception:
            LOG.exception("Ошибка в цикле опроса — продолжаю после паузы")

        elapsed = time.monotonic() - cycle_start
        remaining = max(0.0, cfg.poll_interval - elapsed)
        # спим короткими интервалами, чтобы быстро реагировать на сигнал остановки
        slept = 0.0
        while slept < remaining and not _STOP:
            step = min(0.5, remaining - slept)
            time.sleep(step)
            slept += step

    LOG.info("Конвейер остановлен.")


if __name__ == "__main__":
    main()
