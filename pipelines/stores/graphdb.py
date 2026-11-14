from urllib.parse import quote
from ._http import HTTPGraphStore


class GraphDBStore(HTTPGraphStore):
    def __init__(self, url: str, repository: str, username: str | None = None,
                 password: str | None = None, timeout: int = 90):
        endpoint = url.rstrip("/") + "/repositories/" + quote(repository, safe="") + "/rdf-graphs/service"
        super().__init__(endpoint, username, password, timeout, "GraphDB")
