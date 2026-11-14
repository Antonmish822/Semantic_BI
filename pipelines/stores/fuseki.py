from ._http import HTTPGraphStore


class FusekiStore(HTTPGraphStore):
    def __init__(self, url: str, username: str | None = None, password: str | None = None, timeout: int = 90):
        super().__init__(url.rstrip("/") + "/data", username, password, timeout, "Fuseki")
