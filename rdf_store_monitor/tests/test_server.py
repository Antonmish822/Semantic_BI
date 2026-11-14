from __future__ import annotations

import base64
import json
import sys
import unittest
from urllib.error import URLError
from pathlib import Path
from unittest.mock import MagicMock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import server  # noqa: E402


class StoreClientConfigTests(unittest.TestCase):
    def test_fuseki_legacy_configuration_remains_supported(self) -> None:
        client = server.create_store_client({
            "FUSEKI_URL": "http://fuseki:3030/Persistent/",
            "FUSEKI_USERNAME": "reader",
            "FUSEKI_PASSWORD": "secret",
        })

        self.assertEqual(client.backend, "fuseki")
        self.assertEqual(client.backend_name, "Fuseki")
        self.assertEqual(client.query_url, "http://fuseki:3030/Persistent/query")
        self.assertEqual(client.username, "reader")

    def test_graphdb_uses_repository_query_endpoint(self) -> None:
        client = server.create_store_client({
            "RDF_BACKEND": "graphdb",
            "RDF_URL": "http://graphdb:7200/",
            "GRAPHDB_REPOSITORY": "sales repository",
            "RDF_USERNAME": "monitor",
            "RDF_PASSWORD": "secret",
            "RDF_TIMEOUT": "12",
        })

        self.assertEqual(client.backend, "graphdb")
        self.assertEqual(client.backend_name, "GraphDB")
        self.assertEqual(client.query_url,
                         "http://graphdb:7200/repositories/sales%20repository")
        self.assertEqual(client.username, "monitor")
        self.assertEqual(client.timeout, 12)

    def test_graphdb_requires_repository(self) -> None:
        with self.assertRaisesRegex(ValueError, "GRAPHDB_REPOSITORY"):
            server.create_store_client({"RDF_BACKEND": "graphdb", "RDF_URL": "http://graphdb:7200"})

    def test_unknown_backend_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "RDF_BACKEND"):
            server.create_store_client({"RDF_BACKEND": "other"})


class StoreClientRequestTests(unittest.TestCase):
    def test_select_posts_to_configured_endpoint_with_basic_auth(self) -> None:
        client = server.create_store_client({
            "RDF_BACKEND": "graphdb",
            "RDF_URL": "http://graphdb:7200",
            "GRAPHDB_REPOSITORY": "semantic-bi",
            "RDF_USERNAME": "monitor",
            "RDF_PASSWORD": "secret",
        })
        response = MagicMock()
        response.read.return_value = json.dumps({
            "results": {"bindings": [{"count": {"type": "literal", "value": "7"}}]}
        }).encode()
        response.__enter__.return_value = response

        with patch.object(server, "urlopen", return_value=response) as urlopen:
            rows = client.select("SELECT ?count WHERE {}")

        request = urlopen.call_args.args[0]
        expected_auth = "Basic " + base64.b64encode(b"monitor:secret").decode()
        self.assertEqual(request.full_url, "http://graphdb:7200/repositories/semantic-bi")
        self.assertEqual(request.get_header("Authorization"), expected_auth)
        self.assertEqual(rows[0]["count"]["value"], "7")

    def test_monitor_metrics_have_same_shape_for_graphdb(self) -> None:
        client = server.create_store_client({
            "RDF_BACKEND": "graphdb",
            "RDF_URL": "http://graphdb:7200",
            "GRAPHDB_REPOSITORY": "semantic-bi",
        })
        graph_rows = [{"kind": {"value": "deal"}, "graphs": {"value": "3"}}]
        modified_rows = [{
            "kind": {"value": "deal"},
            "latest": {"value": "2026-11-10T10:00:00Z"},
            "modifiedGraphs": {"value": "2"},
        }]

        with patch.object(client, "select", side_effect=[graph_rows, modified_rows]) as select:
            result = client.monitor_stats()

        self.assertTrue(result["available"])
        self.assertEqual(result["backend"], "graphdb")
        self.assertEqual(result["backend_name"], "GraphDB")
        self.assertEqual(result["types"]["deal"]["graphs"], 3)
        self.assertEqual(result["types"]["deal"]["modified_graphs"], 2)
        self.assertIn("GRAPH ?g { ?s ?p ?o }", select.call_args_list[0].args[0])

    def test_status_keeps_sync_and_s3_when_rdf_store_is_down(self) -> None:
        client = MagicMock(backend="graphdb", backend_name="GraphDB")
        client.monitor_stats.side_effect = URLError("connection refused")
        sync = {"state": "waiting", "last_run_id": "run-1"}
        exporter = {"state": "available", "last_run_id": "run-1", "run_status": "complete"}

        with (patch.object(server, "create_store_client", return_value=client),
              patch.object(server, "loader_status", return_value=sync.copy()),
              patch.object(server, "exporter_status", return_value=exporter)):
            overview = server.get_overview()

        self.assertEqual(overview["sync"]["caught_up"], True)
        self.assertEqual(overview["exporter"], exporter)
        self.assertFalse(overview["store"]["available"])
        self.assertEqual(overview["store"]["backend_name"], "GraphDB")
        self.assertEqual(overview["store"]["error"], "URLError")


if __name__ == "__main__":
    unittest.main()
