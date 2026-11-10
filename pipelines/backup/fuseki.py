from __future__ import annotations

import gzip
import json
import shutil
import time
from pathlib import Path
from urllib.parse import quote, urlsplit, urlunsplit

import requests


class FusekiBackup:
    """Fuseki administrative backup and transactional dataset restore."""

    def __init__(self, dataset_url: str, dataset: str, username: str | None,
                 password: str | None, backup_directory: Path,
                 timeout: int = 90, task_timeout: int = 1800):
        parts = urlsplit(dataset_url.rstrip("/"))
        self.base_url = urlunsplit((parts.scheme, parts.netloc, "", "", ""))
        self.dataset_url = dataset_url.rstrip("/")
        self.dataset = dataset.strip("/")
        if not self.dataset or "/" in self.dataset:
            raise ValueError("fuseki.url должен указывать на один dataset, например /Persistent")
        if bool(username) != bool(password):
            raise ValueError("Для Fuseki задайте оба значения username/password или оставьте оба пустыми")
        self.auth = (username, password) if username and password else None
        self.backup_directory = backup_directory
        self.timeout = (10, max(timeout, 1))
        self.task_timeout = task_timeout

    def _request(self, method: str, path: str, **kwargs) -> requests.Response:
        response = requests.request(method, self.base_url + path, auth=self.auth,
                                    timeout=self.timeout, **kwargs)
        response.raise_for_status()
        return response

    def ensure_access(self) -> dict:
        """Validate Fuseki admin access, target dataset, and server metadata."""
        server = self._request("GET", "/$/server").json()
        datasets = self._request("GET", "/$/datasets").json().get("datasets", [])
        target = next((item for item in datasets
                       if str(item.get("ds.name", "")).strip("/") == self.dataset), None)
        if target is None:
            raise ValueError(f"Fuseki dataset /{self.dataset} не найден или недоступен")
        services = target.get("ds.services", [])
        can_replace = any(
            service.get("srv.type") == "gsp-rw"
            and any(endpoint in {"", "data"} for endpoint in service.get("srv.endpoints", []))
            for service in services
        )
        if not can_replace:
            raise ValueError(f"Fuseki dataset /{self.dataset} не предоставляет GSP write endpoint для восстановления")
        return {"version": server.get("version") or server.get("fusekiVersion")}

    def dataset_stats(self) -> dict[str, int]:
        query = ("SELECT (COUNT(DISTINCT ?g) AS ?graphs) (COUNT(*) AS ?quads) "
                 "WHERE { GRAPH ?g { ?s ?p ?o } }")
        response = self._request(
            "POST", f"/{quote(self.dataset, safe='')}/query",
            data={"query": query}, headers={"Accept": "application/sparql-results+json"})
        bindings = response.json()["results"]["bindings"]
        if len(bindings) != 1:
            raise ValueError("Fuseki statistics query returned an unexpected result")
        row = bindings[0]
        return {"named_graphs": int(row["graphs"]["value"]),
                "named_graph_quads": int(row["quads"]["value"])}

    def _backups_list(self) -> set[str]:
        payload = self._request("GET", "/$/backups-list").json()
        names = payload.get("backups", [])
        if not isinstance(names, list) or not all(isinstance(name, str) for name in names):
            raise ValueError("Fuseki /$/backups-list вернул неожиданный формат")
        return {Path(name).name for name in names}

    def _wait_task(self, task_id: str) -> dict:
        deadline = time.monotonic() + self.task_timeout
        task_url = f"/$/tasks/{quote(task_id, safe='')}"
        while time.monotonic() < deadline:
            payload = self._request("GET", task_url).json()
            tasks = payload if isinstance(payload, list) else [payload]
            if not tasks:
                raise RuntimeError(f"Fuseki не вернул состояние задачи backup {task_id}")
            task = next((item for item in tasks if str(item.get("taskId")) == str(task_id)), tasks[0])
            finished = task.get("finished") or task.get("finishPoint")
            if finished:
                if task.get("success") is not True:
                    raise RuntimeError(f"Fuseki backup task {task_id} завершился с ошибкой: {task}")
                return task
            time.sleep(1)
        raise TimeoutError(f"Fuseki backup task {task_id} не завершилась за {self.task_timeout} секунд")

    def create_backup(self, destination: Path) -> None:
        self.backup_directory.mkdir(parents=True, exist_ok=True)
        before = self._backups_list()
        response = self._request("POST", f"/$/backup/{quote(self.dataset, safe='')}")
        task_id = response.json().get("taskId")
        if not task_id:
            raise ValueError("Fuseki не вернул taskId для backup")
        self._wait_task(str(task_id))
        after = self._backups_list()
        created = after - before
        candidates = [self.backup_directory / name for name in created
                      if name.endswith((".nq.gz", ".nq"))]
        root = self.backup_directory.resolve()
        candidates = [path for path in candidates
                      if path.is_file() and not path.is_symlink() and path.resolve().parent == root]
        if len(candidates) != 1:
            raise RuntimeError(
                "Не удалось однозначно найти новый N-Quads backup Fuseki в "
                f"{self.backup_directory}; новые файлы: {sorted(created)}")
        source = candidates[0]
        if source.suffix == ".gz":
            shutil.copyfile(source, destination)
        else:
            with source.open("rb") as inp, gzip.open(destination, "wb") as out:
                shutil.copyfileobj(inp, out)
        self.verify_backup(destination)

    @staticmethod
    def verify_backup(archive: Path) -> None:
        if not archive.is_file() or archive.stat().st_size == 0:
            raise ValueError(f"Backup Fuseki отсутствует или пуст: {archive}")
        try:
            with gzip.open(archive, "rb") as source:
                # Read through EOF so CRC/truncation errors are detected without
                # retaining the whole dataset in memory.
                while source.read(1024 * 1024):
                    pass
        except (OSError, EOFError) as exc:
            raise ValueError(f"Некорректный gzip N-Quads backup Fuseki: {archive}") from exc

    def restore_backup(self, archive: Path) -> None:
        """Replace the complete dataset using Fuseki's transactional quad PUT."""
        self.verify_backup(archive)
        endpoint = f"{self.dataset_url}/data"
        try:
            with gzip.open(archive, "rb") as source:
                response = requests.put(
                    endpoint, data=source,
                    headers={"Content-Type": "application/n-quads"}, auth=self.auth,
                    timeout=self.timeout)
            response.raise_for_status()
        except requests.RequestException as exc:
            raise RuntimeError(f"Не удалось атомарно заменить Fuseki dataset /{self.dataset}: {exc}") from exc
