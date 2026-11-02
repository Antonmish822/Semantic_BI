from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Dict, Iterable, List

import requests


ENTITY_METHODS: Dict[str, str] = {
    "lead": "crm.lead.list",
    "deal": "crm.deal.list",
    "contact": "crm.contact.list",
    "company": "crm.company.list",
    "activity": "crm.activity.list",
    "productrow": "crm.item.productrow.list",
}


@dataclass(frozen=True)
class BitrixClient:
    webhook_url: str
    timeout: int = 60

    def _endpoint(self, method: str) -> str:
        base = self.webhook_url.rstrip("/") + "/"
        return f"{base}{method}"

    def list_entities(
        self,
        entity: str,
        select: List[str] | None = None,
        filter_: Dict[str, Any] | None = None,
        start: int = 0,
    ) -> Dict[str, Any]:
        method = ENTITY_METHODS[entity]
        payload: Dict[str, Any] = {
            "start": start,
        }
        if select:
            payload["select"] = select
        if filter_:
            payload["filter"] = filter_

        response = requests.post(self._endpoint(method), json=payload, timeout=self.timeout)
        response.raise_for_status()
        return response.json()

    def iter_entities(
        self,
        entity: str,
        select: List[str] | None = None,
        filter_: Dict[str, Any] | None = None,
        batch_size: int = 50,
    ) -> Iterable[List[Dict[str, Any]]]:
        start = 0
        while True:
            data = self.list_entities(entity, select=select, filter_=filter_, start=start)
            items = data.get("result", [])
            if not items:
                break
            yield items
            next_start = data.get("next")
            if next_start is None:
                break
            start = int(next_start)
