from __future__ import annotations

import json
import tarfile
from pathlib import Path
from urllib.parse import quote

import requests


class GraphDBRecovery:
    def __init__(self, base_url: str, repository: str, username: str | None,
                 password: str | None, timeout: int = 90):
        self.base_url = base_url.rstrip("/")
        self.repository = repository
        self.auth = (username, password) if username and password else None
        self.timeout = (10, max(timeout, 600))

    def ensure_admin_access(self) -> bool:
        """Return whether security is enabled; validate admin login if it is."""
        response = requests.get(f"{self.base_url}/rest/security", timeout=(10, 30))
        response.raise_for_status()
        security_enabled = response.json()
        if not isinstance(security_enabled, bool):
            raise ValueError("GraphDB /rest/security returned an unexpected response")
        if not security_enabled:
            return False
        if not self.auth:
            raise PermissionError("GraphDB security включена; задайте GraphDB Recovery admin credentials")

        username, password = self.auth
        login = requests.post(
            f"{self.base_url}/rest/login/{quote(username, safe='')}",
            data=b"", headers={"X-GraphDB-Password": password,
                                "Accept": "application/json"},
            timeout=(10, 30),
        )
        login.raise_for_status()
        payload = login.json()
        authorities = payload.get("grantedAuthorities", payload.get("roles", [])) if isinstance(payload, dict) else []
        if "ROLE_ADMIN" not in authorities and "ADMIN" not in authorities:
            raise PermissionError("Указанная GraphDB Recovery учётная запись не имеет роли администратора")
        return True

    def version(self) -> str | None:
        try:
            response = requests.get(f"{self.base_url}/rest/info/version",
                                    auth=self.auth, timeout=(10, 30))
            response.raise_for_status()
            try:
                payload = response.json()
            except ValueError:
                return response.text.strip() or None
            if isinstance(payload, dict):
                for key in ("version", "productVersion", "graphdbVersion"):
                    if payload.get(key):
                        return str(payload[key])
            return str(payload) if payload else None
        except requests.RequestException:
            return None

    def dataset_stats(self) -> dict[str, int]:
        query = ("SELECT (COUNT(DISTINCT ?g) AS ?graphs) (COUNT(*) AS ?quads) "
                 "WHERE { GRAPH ?g { ?s ?p ?o } }")
        response = requests.post(
            f"{self.base_url}/repositories/{quote(self.repository, safe='')}",
            data={"query": query},
            headers={"Accept": "application/sparql-results+json"},
            auth=self.auth, timeout=self.timeout,
        )
        response.raise_for_status()
        bindings = response.json()["results"]["bindings"]
        if len(bindings) != 1:
            raise ValueError("GraphDB statistics query returned an unexpected result")
        row = bindings[0]
        return {"named_graphs": int(row["graphs"]["value"]),
                "named_graph_quads": int(row["quads"]["value"])}

    def create_backup(self, destination: Path, backup_id: str) -> None:
        response = requests.post(
            f"{self.base_url}/rest/recovery/backup",
            json={"repositories": [self.repository], "backupSystemData": False,
                  "fileName": f"{backup_id}.tar"},
            auth=self.auth, stream=True, timeout=self.timeout,
        )
        response.raise_for_status()
        with response, destination.open("wb") as output:
            for chunk in response.iter_content(chunk_size=1024 * 1024):
                if chunk:
                    output.write(chunk)
            output.flush()

        self.verify_backup(destination)

    @staticmethod
    def verify_backup(archive: Path) -> None:
        if not archive.is_file() or archive.stat().st_size == 0:
            raise ValueError(f"Backup GraphDB отсутствует или пуст: {archive}")
        try:
            with tarfile.open(archive, "r:*") as tar:
                success = any(
                    Path(member.name).name == ".success" or member.name.endswith(".success")
                    for member in tar.getmembers()
                )
        except (tarfile.TarError, OSError) as exc:
            raise ValueError(f"Некорректный TAR backup GraphDB: {archive}") from exc
        if not success:
            raise ValueError("В архиве GraphDB нет маркера .success; backup не считается успешным")

    def restore_backup(self, archive: Path) -> None:
        self.verify_backup(archive)
        params = {"repositories": [self.repository], "restoreSystemData": False,
                  "removeStaleRepositories": False}
        with archive.open("rb") as source:
            response = requests.post(
                f"{self.base_url}/rest/recovery/restore",
                files={
                    "params": (None, json.dumps(params), "application/json"),
                    "file": (archive.name, source, "application/x-tar"),
                },
                auth=self.auth, timeout=self.timeout,
            )
        response.raise_for_status()
