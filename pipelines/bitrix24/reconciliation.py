"""Authoritative snapshot reconciliation for complete Bitrix24 entity types."""
import logging
from ..core.runner import apply_graph_operation

LOG = logging.getLogger("rdf_sync")


def reconcile_snapshot(config, checkpoint, store, present, run_id, manifest, mapping_version):
    for row in checkpoint.entities_missing_from(present, config.reconcile_entity_types):
        apply_graph_operation(
            store, "delete", row["graph_uri"],
            lambda row=row: checkpoint.save_entity(
                row["entity_key"], row["graph_uri"], f"snapshot-delete:{run_id}",
                manifest.get("catchup_to"), True, mapping_version=mapping_version,
            ),
        )
        LOG.info("Удалена сущность, отсутствующая в полном снимке: %s", row["entity_key"])
