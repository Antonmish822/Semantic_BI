from __future__ import annotations

import json
from datetime import datetime
from typing import Any, Dict, List

import click
from dateutil.parser import isoparse

from .bitrix import BitrixClient, ENTITY_METHODS
from .config import load_settings
from .s3 import S3Client, build_raw_key
from .utils import utc_date_str


def _build_filter(entity: str, since: str | None, until: str | None) -> Dict[str, Any] | None:
    if not since and not until:
        return None

    field = {
        "lead": "DATE_CREATE",
        "deal": "DATE_CREATE",
        "contact": "DATE_CREATE",
        "company": "DATE_CREATE",
        "activity": "CREATED",
        "productrow": "DATE_CREATE",
    }[entity]

    filter_: Dict[str, Any] = {}
    if since:
        filter_[f">={field}"] = since
    if until:
        filter_[f"<={field}"] = until
    return filter_


@click.group()
def cli() -> None:
    """Bitrix24 RAW ingest CLI."""


@cli.command("fetch-entity")
@click.option("--entity", type=click.Choice(sorted(ENTITY_METHODS.keys())), required=True)
@click.option("--since", type=str, required=False, help="ISO date, e.g. 2024-01-01")
@click.option("--until", type=str, required=False, help="ISO date, e.g. 2024-01-31")
@click.option("--batch-size", type=int, default=50, show_default=True)
def fetch_entity(entity: str, since: str | None, until: str | None, batch_size: int) -> None:
    settings = load_settings()
    if since:
        isoparse(since)
    if until:
        isoparse(until)

    client = BitrixClient(settings.bitrix_webhook_url)
    s3_client = S3Client(
        endpoint_url=settings.s3_endpoint_url,
        access_key=settings.s3_access_key,
        secret_key=settings.s3_secret_key,
        region=settings.s3_region,
        use_ssl=settings.s3_use_ssl,
    )

    filter_ = _build_filter(entity, since, until)
    date_str = utc_date_str()

    batch_index = 1
    total = 0
    for batch in client.iter_entities(entity, filter_=filter_, batch_size=batch_size):
        payload = {
            "meta": {
                "entity": entity,
                "method": ENTITY_METHODS[entity],
                "batch_index": batch_index,
                "extracted_at": datetime.utcnow().isoformat() + "Z",
                "filter": filter_,
                "count": len(batch),
            },
            "items": batch,
        }
        key = build_raw_key(settings.s3_prefix, entity, date_str, batch_index)
        s3_client.put_json(settings.s3_bucket, key, payload)
        total += len(batch)
        batch_index += 1

    click.echo(f"Uploaded {total} {entity} records to s3://{settings.s3_bucket}/{settings.s3_prefix}")


if __name__ == "__main__":
    cli()
