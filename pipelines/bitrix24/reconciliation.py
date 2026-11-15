"""Authoritative snapshot reconciliation for complete Bitrix24 entity types."""
import logging
from ..core.runner import apply_graph_operation

LOG = logging.getLogger("rdf_sync")


def reconcile_snapshot(config, checkpoint, store, present, run_id, manifest, mapping_version,
                       entity_types=None):
    entity_types = config.reconcile_entity_types if entity_types is None else entity_types
    deleted = 0
    for row in checkpoint.entities_missing_from(present, entity_types):
        apply_graph_operation(
            store, "delete", row["graph_uri"],
            lambda row=row: checkpoint.save_entity(
                row["entity_key"], row["graph_uri"], f"snapshot-delete:{run_id}",
                manifest.get("catchup_to"), True, mapping_version=mapping_version,
            ),
        )
        deleted += 1
    LOG.info("Snapshot reconciliation completed: deleted_graphs=%d entity_types=%s",
             deleted, ",".join(sorted(entity_types)) or "none")
    return deleted
