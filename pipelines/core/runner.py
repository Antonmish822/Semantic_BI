"""Apply a named graph operation before recording its successful outcome."""
from collections.abc import Callable
from .graph_store import GraphStore


def apply_graph_operation(store: GraphStore, operation: str, graph_uri: str,
                          on_success: Callable[[], None], rdf_bytes: bytes | None = None) -> None:
    if operation == "replace":
        if rdf_bytes is None:
            raise ValueError("replace requires RDF bytes")
        store.replace_graph(graph_uri, rdf_bytes)
    elif operation == "delete":
        store.delete_graph(graph_uri)
    else:
        raise ValueError(f"Unknown graph operation: {operation}")
    on_success()
