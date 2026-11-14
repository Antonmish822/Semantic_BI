import requests


class HTTPGraphStore:
    def __init__(self, endpoint: str, username: str | None, password: str | None, timeout: int, label: str):
        self.endpoint = endpoint
        self.auth = (username, password) if username else None
        self.timeout = timeout
        self.label = label

    def _request(self, method: str, graph_uri: str, body: bytes | None = None) -> None:
        if not graph_uri:
            raise ValueError("graph_uri обязателен")
        try:
            response = requests.request(
                method, self.endpoint, params={"graph": graph_uri}, data=body,
                headers={"Content-Type": "application/rdf+xml; charset=utf-8"} if body is not None else None,
                auth=self.auth, timeout=self.timeout,
            )
        except requests.RequestException as exc:
            raise RuntimeError(f"{self.label} {method} graph failed: {exc}") from exc
        allowed = (200, 201, 204) if method == "PUT" else (200, 204, 404)
        if response.status_code not in allowed:
            raise RuntimeError(f"{self.label} {method} graph failed: HTTP {response.status_code}: {response.text[:500]}")

    def replace_graph(self, graph_uri: str, rdf_bytes: bytes) -> None:
        self._request("PUT", graph_uri, rdf_bytes)

    def delete_graph(self, graph_uri: str) -> None:
        self._request("DELETE", graph_uri)
