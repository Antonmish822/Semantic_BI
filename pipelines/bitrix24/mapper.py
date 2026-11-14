from __future__ import annotations
import hashlib
import json
import re
from copy import deepcopy
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from typing import Any
from .config import Config
try:
    import cccr_py as cccr
except ImportError:
    cccr = None

URI_ID_FIELDS = frozenset({"ASSIGNED_BY_ID", "COMPANY_ID", "CONTACT_ID", "ID",
                           "LAST_ACTIVITY_BY", "LEAD_ID", "OWNER_ID", "RESPONSIBLE_ID"})


def normalize_uri_id(value: Any) -> Any:
    # Bitrix can return numeric identifiers as strings with surrounding space.
    # Trim those before they are interpolated into an RDF IRI.
    return value.strip() if isinstance(value, str) else value

def append_xml_value(parent: ET.Element, name: str, value: Any) -> None:
    node = ET.SubElement(parent, name)
    if value is None:
        node.set("nil", "true")
    elif isinstance(value, dict):
        for key, child in value.items():
            append_xml_value(node, str(key), child)
    elif isinstance(value, list):
        for child in value:
            append_xml_value(node, "item", child)
    elif isinstance(value, bool):
        node.text = "true" if value else "false"
    else:
        node.text = str(value)


def event_to_xml(event: dict[str, Any]) -> str:
    payload = event.get("payload")
    if not isinstance(payload, dict):
        raise ValueError("В событии отсутствует объект payload")
    root = ET.Element("json")
    record = ET.SubElement(root, "object")
    for field, value in payload.items():
        append_xml_value(record, field, normalize_uri_id(value) if field in URI_ID_FIELDS else value)
    bindings = ET.SubElement(record, "bindings")
    seen_bindings = set()
    for binding in event.get("bindings") or []:
        entity_type = normalize_uri_id(binding.get("entityTypeId"))
        entity_id = normalize_uri_id(binding.get("entityId"))
        binding_key = (str(entity_type), str(entity_id))
        if binding_key in seen_bindings:
            continue
        seen_bindings.add(binding_key)
        item = ET.SubElement(bindings, "item")
        append_xml_value(item, "entityTypeId", entity_type)
        append_xml_value(item, "entityId", entity_id)
    ET.indent(root, space="  ")
    return ET.tostring(root, encoding="utf-8", xml_declaration=True).decode("utf-8")


@dataclass(frozen=True)
class MappingEntry:
    path: Path
    pattern: re.Pattern[str]
    match_mode: str
    key: str

    def matches(self, relative_key: str) -> bool:
        return getattr(self.pattern, self.match_mode)(relative_key) is not None


def load_mappings(config: Config) -> list[MappingEntry]:
    registry = json.loads(config.registry_path.read_text(encoding="utf-8"))
    if not isinstance(registry, list):
        raise ValueError("Mapping registry должен быть JSON-массивом")
    entries = []
    for index, item in enumerate(registry, 1):
        if not isinstance(item, dict):
            raise ValueError(f"Некорректная запись mapping registry №{index}")
        schema_name = item.get("manifest") or item.get("schema")
        pattern = item.get("pattern")
        if not isinstance(schema_name, str) or not isinstance(pattern, str):
            raise ValueError(f"В mapping registry №{index} нужны строки manifest/schema и pattern")
        schema_path = (config.schemas_root / schema_name).resolve()
        if not schema_path.is_relative_to(config.schemas_root) or not schema_path.is_file():
            raise ValueError(f"Файл схемы не найден или вышел за schemas_root: {schema_path}")
        flags = 0
        for flag in item.get("flags", ""):
            flags |= {"i": re.IGNORECASE, "m": re.MULTILINE, "s": re.DOTALL}.get(flag, 0)
        match_mode = item.get("match", "search")
        if match_mode not in {"search", "match", "fullmatch"}:
            raise ValueError(f"Неизвестный match mode: {match_mode!r}")
        entries.append(MappingEntry(schema_path, re.compile(pattern, flags), match_mode, schema_name))
    return entries


def mapping_fingerprint(config: Config, mappings: list[MappingEntry]) -> str:
    digest = hashlib.sha256()
    digest.update(config.registry_path.read_bytes())
    for entry in sorted(mappings, key=lambda value: value.key):
        digest.update(entry.key.encode("utf-8"))
        digest.update(entry.path.read_bytes())
    return digest.hexdigest()


def transform_event(config: Config, mappings: list[MappingEntry], event: dict[str, Any],
                    relative_key: str) -> bytes:
    if cccr is None:
        raise RuntimeError("Не найден cccr_py. Установите собранную библиотеку CCCR в это Python-окружение.")
    selected = [entry for entry in mappings if entry.matches(relative_key)]
    if not selected:
        raise ValueError(f"Нет CCCR mapping для {relative_key}")
    ctx = cccr.Context()
    xml_text = event_to_xml(event)
    source = ctx.load_xml(xml_text)
    if source is None:
        raise RuntimeError(f"CCCR не загрузил payload: {ctx.last_error}")
    combined = None
    for entry in selected:
        schema_document = json.loads(entry.path.read_text(encoding="utf-8"))
        description = schema_document.get("description") or {}
        properties = description.get("properties") or []
        payload = event.get("payload") or {}
        retained_properties = []
        binding_properties = {}
        for prop in properties:
            literal = prop.get("literal") or {}
            compute = literal.get("compute") or {}
            expression = compute.get("xquery")
            datatype = prop.get("datatype")
            direct_field = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_.-]*)/text\(\)", expression or "")
            if datatype and direct_field:
                raw_value = payload.get(direct_field.group(1))
                if raw_value is None or raw_value == "":
                    # CCCR validates typed literals even when text() selects no value;
                    # omitting a null/missing typed property avoids an invalid empty lexical form.
                    continue
            uri_compute = (prop.get("URI") or {}).get("compute") or {}
            uri_expression = uri_compute.get("xquery")
            binding_type = re.search(
                r"entityTypeId/text\(\)\s*=\s*'([0-9]+)'", uri_expression or ""
            )
            if binding_type:
                binding_properties[binding_type.group(1)] = prop
                continue
            optional_uri = re.fullmatch(
                r"if \(([A-Za-z_][A-Za-z0-9_]*)/text\(\) != ''\) then .+ else \(\)",
                uri_expression or "",
            )
            if optional_uri and payload.get(optional_uri.group(1)) in (None, ""):
                # CCCR currently emits an invalid empty URI for an empty sequence
                # returned by this optional-property pattern. Omit the link instead.
                continue
            retained_properties.append(prop)
        if "properties" in description:
            description["properties"] = retained_properties
        schema = ctx.load_schema(json.dumps(schema_document, ensure_ascii=False),
                                 include_base=str(entry.path.parent))
        if schema is None:
            raise RuntimeError(f"CCCR не загрузил схему {entry.path}: {ctx.last_error}")
        graph = ctx.run(schema, source)
        if graph is None:
            raise RuntimeError(f"CCCR не применил схему {entry.path}: {ctx.last_error}")
        if combined is None:
            combined = graph
        elif not combined.merge(graph):
            raise RuntimeError(f"Не удалось объединить результаты схем: {ctx.last_error}")
        if binding_properties:
            # CCCR joins a multi-item XQuery URI sequence into one space-separated
            # IRI. Render each lead/deal binding independently so every CRM link
            # becomes its own RDF triple.
            seen_bindings = set()
            for binding in event.get("bindings") or []:
                entity_type = str(normalize_uri_id(binding.get("entityTypeId")))
                prop = binding_properties.get(entity_type)
                if prop is None:
                    continue
                entity_id = normalize_uri_id(binding.get("entityId"))
                binding_key = (entity_type, str(entity_id))
                if binding_key in seen_bindings:
                    continue
                seen_bindings.add(binding_key)
                if not str(entity_id).isdigit():
                    continue
                binding_event = dict(event, bindings=[{"entityTypeId": entity_type,
                                                       "entityId": str(entity_id)}])
                binding_source = ctx.load_xml(event_to_xml(binding_event))
                binding_schema_document = deepcopy(schema_document)
                binding_schema_document["description"]["properties"] = [prop]
                binding_schema = ctx.load_schema(json.dumps(binding_schema_document, ensure_ascii=False),
                                                 include_base=str(entry.path.parent))
                if binding_schema is None:
                    raise RuntimeError(f"CCCR не загрузил binding-схему {entry.path}: {ctx.last_error}")
                binding_graph = ctx.run(binding_schema, binding_source)
                if binding_graph is None:
                    raise RuntimeError(f"CCCR не применил binding-схему {entry.path}: {ctx.last_error}")
                if not combined.merge(binding_graph):
                    raise RuntimeError(f"Не удалось объединить activity bindings: {ctx.last_error}")
    rdfxml = combined.to_rdfxml()
    if rdfxml is None:
        raise RuntimeError(f"CCCR не сериализовал RDF/XML: {ctx.last_error}")
    return rdfxml.encode("utf-8")
