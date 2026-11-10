#!/usr/bin/env python3
"""Read-only local dashboard for named graphs in Apache Jena Fuseki."""
from __future__ import annotations

import base64
import json
import os
import threading
import time
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import quote_plus, urlsplit
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent
CACHE_SECONDS = 20
CACHE: dict[str, object] = {"at": 0.0, "payload": None}
CACHE_LOCK = threading.Lock()


def load_dotenv() -> None:
    path = Path(os.environ.get("DASHBOARD_ENV_FILE", ROOT / ".env"))
    if not path.exists():
        return
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key, value = key.strip(), value.strip()
        if value[:1] in {"'", '"'} and value[-1:] == value[:1]:
            value = value[1:-1]
        os.environ.setdefault(key, value)


def sparql_literal(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def load_query(name: str, graph_prefix: str) -> str:
    template = (ROOT / "queries" / name).read_text(encoding="utf-8")
    marker = "{{graph_prefix}}"
    if template.count(marker) != 1:
        raise ValueError(f"SPARQL template {name} must contain exactly one graph prefix marker")
    return template.replace(marker, sparql_literal(graph_prefix))


class FusekiClient:
    def __init__(self) -> None:
        self.base_url = os.environ.get("FUSEKI_URL", "http://127.0.0.1:3030/Persistent").rstrip("/")
        self.username = os.environ.get("FUSEKI_USERNAME", "")
        self.password = os.environ.get("FUSEKI_PASSWORD", "")
        self.graph_prefix = os.environ.get("GRAPH_PREFIX", "urn:bitrix24:entity:")
        self.timeout = float(os.environ.get("FUSEKI_TIMEOUT", "30"))

    def select(self, query: str) -> list[dict[str, dict[str, str]]]:
        body = ("query=" + quote_plus(query)).encode()
        headers = {
            "Accept": "application/sparql-results+json",
            "Content-Type": "application/x-www-form-urlencoded; charset=utf-8",
        }
        if self.username:
            token = base64.b64encode(f"{self.username}:{self.password}".encode()).decode("ascii")
            headers["Authorization"] = f"Basic {token}"
        request = Request(self.base_url + "/query", data=body, headers=headers, method="POST")
        with urlopen(request, timeout=self.timeout) as response:
            result = json.loads(response.read().decode("utf-8"))
        return result.get("results", {}).get("bindings", [])

    def stats(self) -> dict[str, object]:
        graph_query = load_query("graphs_in_scope.rq", self.graph_prefix)
        date_query = load_query("graphs_by_created_at.rq", self.graph_prefix)
        graph_rows = self.select(graph_query)
        date_rows = self.select(date_query)
        counts: dict[str, int] = {}
        for row in graph_rows:
            graph = row["g"]["value"]
            tail = graph[len(self.graph_prefix):]
            entity_type = tail.split(":", 1)[0] if ":" in tail else "unknown"
            if not entity_type.replace("_", "").replace("-", "").isalnum():
                entity_type = "other"
            counts[entity_type] = counts.get(entity_type, 0) + 1
        timeline = []
        for row in date_rows:
            try:
                count = int(row["graphs"]["value"])
            except (KeyError, ValueError):
                continue
            timeline.append({"date": row["day"]["value"], "graphs": count})
        return {
            "total_graphs": len(graph_rows),
            "dated_graphs": sum(item["graphs"] for item in timeline),
            "by_type": dict(sorted(counts.items(), key=lambda item: (-item[1], item[0]))),
            "by_date": timeline,
            "date_predicate": "createdAt",
            "updated_at": datetime.now(timezone.utc).isoformat(),
        }

    def monitor_stats(self) -> dict[str, object]:
        query = """SELECT ?kind (COUNT(DISTINCT ?g) AS ?graphs) WHERE {
  VALUES (?kind ?marker) { ("deal" ":deal:") ("activity" ":activity:")
                           ("user" ":user:") ("category" ":category:")
                           ("stage" ":stage:") ("source" ":source:") }
  GRAPH ?g {}
  FILTER(STRSTARTS(STR(?g), {{graph_prefix}}) && CONTAINS(STR(?g), ?marker))
} GROUP BY ?kind"""
        query = query.replace("{{graph_prefix}}", sparql_literal(self.graph_prefix))
        rows = self.select(query)
        types = {}
        for row in rows:
            kind = row["kind"]["value"]
            types[kind] = {"graphs": int(row["graphs"]["value"]),
                           "modified_graphs": 0, "latest_modified_at": None}
        latest_query = """PREFIX xsd: <http://www.w3.org/2001/XMLSchema#>
SELECT ?kind (MAX(?dt) AS ?latest) (COUNT(DISTINCT ?g) AS ?modifiedGraphs) WHERE {
  VALUES (?kind ?marker) { ("deal" ":deal:") ("activity" ":activity:")
                           ("user" ":user:") ("category" ":category:")
                           ("stage" ":stage:") ("source" ":source:") }
  GRAPH ?g { ?s <https://example.org/bitrix24#modifiedAt> ?raw . }
  FILTER(STRSTARTS(STR(?g), {{graph_prefix}}) && CONTAINS(STR(?g), ?marker))
  FILTER(isLiteral(?raw) && STR(?raw) != "")
  BIND(xsd:dateTime(STR(?raw)) AS ?dt)
  FILTER(BOUND(?dt))
} GROUP BY ?kind""".replace("{{graph_prefix}}", sparql_literal(self.graph_prefix))
        for row in self.select(latest_query):
            kind = row["kind"]["value"]
            if kind in types:
                types[kind]["modified_graphs"] = int(row["modifiedGraphs"]["value"])
                types[kind]["latest_modified_at"] = row.get("latest", {}).get("value")
        return {"available": True, "types": types, "checked_at": datetime.now(timezone.utc).isoformat()}


def loader_status() -> dict[str, object]:
    path = Path(os.environ.get("LOADER_STATUS_PATH", ""))
    if not str(path) or not path.is_file():
        return {"state": "unknown", "reason": "status_file_missing"}
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
        updated = datetime.fromisoformat(value["updated_at"].replace("Z", "+00:00"))
        age = max(0, int((datetime.now(timezone.utc) - updated).total_seconds()))
        if value.get("state") in {"running", "waiting"} and age > 180:
            value["state"] = "stale"
        value["age_seconds"] = age
        return value
    except (OSError, ValueError, KeyError, TypeError):
        return {"state": "unknown", "reason": "status_file_invalid"}


def exporter_status() -> dict[str, object]:
    bucket = os.environ.get("S3_BUCKET", "")
    portal = os.environ.get("S3_PORTAL_ID", "")
    if not bucket or not portal:
        return {"state": "unknown", "reason": "s3_config_missing"}
    try:
        import boto3
        client = boto3.client(
            "s3", endpoint_url=os.environ.get("S3_ENDPOINT_URL") or None,
            region_name=os.environ.get("S3_REGION", "ru-central1"),
            aws_access_key_id=os.environ.get("AWS_ACCESS_KEY_ID") or None,
            aws_secret_access_key=os.environ.get("AWS_SECRET_ACCESS_KEY") or None,
        )
        prefix = f"bitrix24/{portal}/"
        state = json.loads(client.get_object(Bucket=bucket, Key=prefix + "state.json")["Body"].read())
        run_id = state.get("last_run_id")
        if not isinstance(run_id, str) or not run_id:
            return {"state": "unknown", "reason": "exporter_state_has_no_run"}
        run_prefix = f"{prefix}runs/{run_id}/"
        manifest = json.loads(client.get_object(Bucket=bucket, Key=run_prefix + "manifest.json")["Body"].read())
        try:
            marker = json.loads(client.get_object(Bucket=bucket, Key=run_prefix + "status.json")["Body"].read())
            run_status = marker.get("status", "unknown")
        except Exception:
            run_status = "missing"
        return {"state": "available", "last_run_id": run_id,
                "run_status": run_status, "mode": manifest.get("mode"),
                "created_at": manifest.get("created_at"),
                "checked_at": datetime.now(timezone.utc).isoformat()}
    except Exception as exc:
        return {"state": "unavailable", "reason": type(exc).__name__,
                "checked_at": datetime.now(timezone.utc).isoformat()}


def get_overview() -> dict[str, object]:
    fuseki = FusekiClient()
    started = time.monotonic()
    store = fuseki.monitor_stats()
    store["query_seconds"] = round(time.monotonic() - started, 3)
    sync = loader_status()
    exporter = exporter_status()
    checkpoint_run = sync.get("last_run_id")
    exporter_run = exporter.get("last_run_id")
    if exporter.get("state") == "available" and exporter.get("run_status") == "complete":
        sync["caught_up"] = checkpoint_run == exporter_run if isinstance(checkpoint_run, str) else None
        sync["exporter_run_id"] = exporter_run
        sync["exporter_run_status"] = exporter.get("run_status")
    return {"sync": sync, "exporter": exporter, "store": store,
            "checked_at": datetime.now(timezone.utc).isoformat()}


def get_stats() -> dict[str, object]:
    now = time.monotonic()
    with CACHE_LOCK:
        cached = CACHE.get("payload")
        if cached is not None and now - float(CACHE["at"]) < CACHE_SECONDS:
            return cached  # type: ignore[return-value]
        payload = FusekiClient().stats()
        CACHE["at"] = now
        CACHE["payload"] = payload
        return payload


class Handler(BaseHTTPRequestHandler):
    server_version = "SemanticBIGraphDashboard/1.0"

    def do_GET(self) -> None:  # noqa: N802
        path = urlsplit(self.path).path
        if path in {"/", "/index.html"}:
            self._send_file(ROOT / "static" / "index.html", "text/html; charset=utf-8")
        elif path == "/api/stats":
            try:
                body = json.dumps(get_stats(), ensure_ascii=False).encode("utf-8")
                self._send(200, body, "application/json; charset=utf-8")
            except (HTTPError, URLError, TimeoutError, ValueError, KeyError, OSError) as exc:
                # Do not return Fuseki error bodies or credentials to the browser.
                message = "Fuseki недоступен или вернул ошибку запроса статистики"
                self._send(502, json.dumps({"error": message}, ensure_ascii=False).encode(), "application/json; charset=utf-8")
                self.log_error("Fuseki stats query failed (%s)", type(exc).__name__)
        elif path == "/api/status":
            try:
                body = json.dumps(get_overview(), ensure_ascii=False).encode("utf-8")
                self._send(200, body, "application/json; charset=utf-8")
            except (HTTPError, URLError, TimeoutError, ValueError, KeyError, OSError) as exc:
                self._send(502, json.dumps({"error": "Ошибка чтения состояния Fuseki"}, ensure_ascii=False).encode("utf-8"), "application/json; charset=utf-8")
                self.log_error("Fuseki monitor query failed (%s)", type(exc).__name__)
        else:
            self._send(404, b"Not found", "text/plain; charset=utf-8")

    def do_POST(self) -> None:  # noqa: N802
        self._send(405, b"Read-only dashboard", "text/plain; charset=utf-8")

    def do_PUT(self) -> None:  # noqa: N802
        self._send(405, b"Read-only dashboard", "text/plain; charset=utf-8")

    def do_DELETE(self) -> None:  # noqa: N802
        self._send(405, b"Read-only dashboard", "text/plain; charset=utf-8")

    def _send_file(self, path: Path, content_type: str) -> None:
        self._send(200, path.read_bytes(), content_type)

    def _send(self, status: int, body: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt: str, *args: object) -> None:
        print(f"{self.log_date_time_string()} {self.address_string()} {fmt % args}")


def main() -> None:
    load_dotenv()
    host = os.environ.get("DASHBOARD_HOST", "127.0.0.1")
    port = int(os.environ.get("DASHBOARD_PORT", "8080"))
    server = ThreadingHTTPServer((host, port), Handler)
    print(f"Dashboard: http://{host}:{port} (read-only; Fuseki: {os.environ.get('FUSEKI_URL', 'http://127.0.0.1:3030/Persistent')})")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("Stopping dashboard")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
