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
    path = ROOT / ".env"
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
