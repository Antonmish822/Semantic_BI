from __future__ import annotations
import hashlib
from urllib.parse import quote
SUPPORTED_ENTITIES = {"deal", "activity", "user", "category", "stage", "source"}

def entity_graph_uri(namespace: str, entity_key: str) -> str:
    parts = entity_key.split("/", 2)
    if len(parts) != 3 or not all(parts):
        raise ValueError(f"Некорректный entity_key: {entity_key!r}")
    portal, entity_type, _identity = parts
    if entity_type not in SUPPORTED_ENTITIES:
        raise ValueError(f"Неизвестный тип сущности в entity_key: {entity_type!r}")
    portal_part = quote(portal, safe="-._~")
    type_part = quote(entity_type, safe="-._~")
    digest = hashlib.sha256(entity_key.encode("utf-8")).hexdigest()
    return f"{namespace.rstrip(':') or namespace}:{portal_part}:{type_part}:{digest}"
