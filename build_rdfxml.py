#!/usr/bin/env python3
"""Build JSON -> XML -> Turtle using settings from the adjacent .env file.

Run with: uv run bitrix24_cccr_examples/build_rdfxml.py
"""
from __future__ import annotations

import argparse
from contextlib import nullcontext
import json
import os
import re
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve().parent


def load_dotenv(path: Path) -> dict[str, str]:
    """Read KEY=VALUE lines without dependencies; process environment wins."""
    if not path.is_file():
        raise FileNotFoundError(f"Не найден {path}; создайте файл по образцу .env.example")
    config: dict[str, str] = {}
    for number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            raise ValueError(f"Некорректная строка {number} в {path}; ожидалось KEY=VALUE")
        key, value = (part.strip() for part in line.split("=", 1))
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        config[key] = os.environ.get(key, value)
    return config


def config_path(config: dict[str, str], key: str, base: Path) -> Path:
    value = config.get(key, "").strip()
    if not value:
        raise ValueError(f"В .env не задан параметр {key}")
    path = Path(value).expanduser()
    return path.resolve() if path.is_absolute() else (base / path).resolve()


def append_value(parent: ET.Element, name: str, value: Any) -> None:
    element = ET.SubElement(parent, name)
    if value is None:
        element.set("nil", "true")
    elif isinstance(value, dict):
        for key, child in value.items():
            append_value(element, str(key), child)
    elif isinstance(value, list):
        for child in value:
            append_value(element, "item", child)
    elif isinstance(value, bool):
        element.text = "true" if value else "false"
    else:
        element.text = str(value)


def make_xml(source: Path, json_dir: Path, xml_dir: Path) -> Path:
    relative = source.relative_to(json_dir)
    payload = json.loads(source.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise ValueError(f"Ожидался JSON-объект: {source}")
    root = ET.Element("json")
    record = ET.SubElement(root, "object")
    for field, value in payload.items():
        append_value(record, field, value)
    ET.indent(root, space="  ")
    output = xml_dir / relative.with_suffix(".xml")
    output.parent.mkdir(parents=True, exist_ok=True)
    ET.ElementTree(root).write(output, encoding="utf-8", xml_declaration=True)
    return output


def load_registry(path: Path) -> list[tuple[re.Pattern[str], Path]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, list):
        raise ValueError(f"Реестр схем должен быть JSON-массивом: {path}")
    entries = []
    for index, item in enumerate(data, 1):
        if not isinstance(item, dict) or not isinstance(item.get("pattern"), str) or not isinstance(item.get("schema"), str):
            raise ValueError(f"Запись реестра №{index} должна содержать строки 'pattern' и 'schema'")
        schema_path = (path.parent / item["schema"]).resolve()
        if not schema_path.is_file():
            raise FileNotFoundError(f"Схема из записи №{index} не найдена: {schema_path}")
        try:
            entries.append((re.compile(item["pattern"]), schema_path))
        except re.error as exc:
            raise ValueError(f"Некорректное регулярное выражение в записи №{index}: {exc}") from exc
    return entries


def run_schema(cccr: Any, schema_path: Path, xml_path: Path, relative: Path, rdf_dir: Path) -> None:
    output_group = relative.parent.name or relative.stem
    output_path = rdf_dir / output_group / f"{relative.stem}_{schema_path.stem}.rdf"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    context = cccr.Context()
    schema_text = schema_path.read_text(encoding="utf-8")
    schema = context.load_schema(schema_text, include_base=str(schema_path.parent))
    if schema is None:
        raise RuntimeError(f"Не удалось загрузить {schema_path}: {context.last_error}")
    source = context.load_xml(xml_path.read_text(encoding="utf-8"))
    if source is None:
        raise RuntimeError(f"Не удалось загрузить XML {xml_path}: {context.last_error}")
    graph = context.run(schema, source)
    if graph is None:
        raise RuntimeError(f"Ошибка выполнения {schema_path.name} на {relative}: {context.last_error}")
    rdfxml = graph.to_rdfxml()
    if rdfxml is None:
        raise RuntimeError(f"Ошибка RDF/XML-сериализации {relative}: {context.last_error}")
    output_path.write_text(rdfxml, encoding="utf-8")
    print(f"{schema_path.name}: {output_path}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Преобразование Bitrix24 JSON → XML → RDF/XML через cccr_py")
    parser.add_argument("--env-file", type=Path, default=HERE / ".env", help="Путь к конфигурации .env")
    parser.add_argument("--cccr-pythonpath", type=Path, help="Переопределить каталог модуля cccr_py из .env")
    parser.add_argument("--registry", type=Path, help="Переопределить SCHEMA_REGISTRY из .env")
    args = parser.parse_args()
    try:
        env_file = args.env_file.resolve()
        config = load_dotenv(env_file)
        base = env_file.parent
        data_root = config_path(config, "DATA_ROOT", base)
        xml_dir = config_path(config, "XML_OUTPUT_DIR", base)
        rdf_dir = config_path(config, "RDF_OUTPUT_DIR", base)
        save_xml_value = config.get("SAVE_INTERMEDIATE_XML", "true").strip().lower()
        if save_xml_value not in {"true", "false"}:
            raise ValueError("SAVE_INTERMEDIATE_XML принимает только true или false")
        save_xml = save_xml_value == "true"
        registry_path = args.registry.resolve() if args.registry else config_path(config, "SCHEMA_REGISTRY", base)
        binding_path = args.cccr_pythonpath.resolve() if args.cccr_pythonpath else config_path(config, "CCCR_PYTHONPATH", base)
        sys.path.insert(0, str(binding_path))
        try:
            import cccr_py as cccr
        except ImportError as exc:
            raise RuntimeError(f"Не удалось импортировать cccr_py из {binding_path}: {exc}") from exc
        registry = load_registry(registry_path)
        files = sorted(data_root.rglob("*.json"))
        if not files:
            raise ValueError(f"В {data_root} нет JSON-файлов")
        for source in files:
            relative = source.relative_to(data_root)
            matched = [schema for pattern, schema in registry if pattern.search(relative.as_posix())]
            if not matched:
                print(f"Пропуск без схемы: {relative.as_posix()}")
                continue
            temp_context = nullcontext(str(xml_dir)) if save_xml else tempfile.TemporaryDirectory(prefix="bitrix24_xml_")
            with temp_context as xml_root:
                xml_path = make_xml(source, data_root, Path(xml_root))
                print(f"{relative.as_posix()} → {xml_path}")
                for schema in matched:
                    run_schema(cccr, schema, xml_path, relative, rdf_dir)
    except (OSError, ValueError, KeyError, RuntimeError, json.JSONDecodeError) as exc:
        print(f"Ошибка: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
