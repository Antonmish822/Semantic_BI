# Sanitized Bitrix24 event payload fixtures

These six payloads preserve the field names and JSON value types of one event
per supported mapping from
`bitrix24_s3_export/local-data/snapshot-2026-11-04-v3-ready/objects/bitrix24/garpix-bitrix24-ru/events/`.
They are based on real exporter records, but names, titles, IDs, contact/user
references, and source/stage codes have been replaced with synthetic values.
No raw event envelope, event hash, or personal/business text is copied.

The fixtures intentionally use the per-entity payload shape emitted by the
exporter. In particular, category, stage, and source events are individual
records, while the legacy CCCR schemas also supported the older aggregate
dictionary document shape. Their selectors therefore cover both forms.

`expected.json` lists one RDF subject and one predicate/value pair per fixture.
The native CCCR test checks these expectations when `cccr_py` is installed; the
default unit suite still validates the fixture shape and mapping selection
without native CCCR dependencies.
