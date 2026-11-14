from typing import Protocol


class GraphStore(Protocol):
    """The two named graph operations required by the loader."""

    def replace_graph(self, graph_uri: str, rdf_bytes: bytes) -> None: ...

    def delete_graph(self, graph_uri: str) -> None: ...
