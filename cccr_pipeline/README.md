# Bitrix24 S3 manifests → named Fuseki graphs

`cccr_pipeline.py` consumes the event contract from `bitrix24_s3_export`.
It does not process arbitrary files found under `events/`: it applies the
adopted complete snapshot first, then completed incremental run manifests.

## Graph identity and update behavior

Each CRM entity has one stable graph URI:

```text
urn:bitrix24:entity:<portal>:<entity-type>:<sha256(entity_key)>
```

For example, every version of the same deal in the same portal maps to the
same graph. On each accepted `upsert`, CCCR builds RDF from the event payload
and the loader replaces the complete named graph using Graph Store `PUT`.
Several matching CCCR schemas are merged before that `PUT`.

The loader verifies manifest references and event SHA-256 values. It waits for
both `manifest.status == complete` and the incremental run's `status.json` to
report `complete`. Successfully applied entity versions and run IDs are stored
in a local SQLite checkpoint. Reprocessing after a crash is idempotent.
The checkpoint includes a fingerprint of the mapping registry and schemas;
changing a mapping reprojects current entities from their source events.
An OS file lock prevents two loader processes from applying runs concurrently
against the same checkpoint.

The local snapshot is authoritative for the configured entity types in
`snapshot_reconcile_entity_types`. By default these are `deal`, `user`,
`category`, `stage`, and `source`. Activities are excluded because the exporter
snapshot contains only the recent 90-day window. Incremental backfills are
ignored unless an explicit workflow is added for them.

## Install and configure

Build and install `cccr_py` from the bundled CCCR engine first. Then install
the Python dependencies:

```bash
python -m pip install -r requirements.txt
```

Copy `config.example.yaml` to `config.yaml` and set the S3 bucket, portal,
credentials, and Fuseki dataset URL. The mapping registry is local at
`cccr_pipeline/manifests_registry.json`; its schema paths resolve under
`../data` by default. Keep the SQLite checkpoint path stable between runs.
Do not commit `config.yaml` or credentials.

Run one cycle:

```bash
python cccr_pipeline.py --once
```

Run continuously (polling interval comes from the config):

```bash
python cccr_pipeline.py
```

The S3 reader needs list/get access to the configured portal prefix. Fuseki
needs Graph Store `PUT` and `DELETE` access. Use a persistent Fuseki dataset;
the bundled Compose example uses in-memory storage and is only for local
experimentation.

## Deletions and limits

Deleting an old version file from S3 does not delete the entity graph if a
newer event for the same `entity_key` exists. A graph is deleted when the
exporter publishes a `delete` event or when a new complete snapshot omits an
entity type configured for authoritative reconciliation.

The current exporter does not publish tombstones for records deleted directly
in Bitrix24. That requires an exporter change or an agreed full reconciliation
source. Do not interpret absence from an incremental manifest as deletion.

Changing the graph URI scheme does not automatically remove graphs created by
the previous loader. Before cutover, load and reconcile the new snapshot, then
remove old `urn:cccr:*` graphs using a reviewed, namespace-scoped cleanup.
