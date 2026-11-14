"""Contract 2 loader tests use synthetic payloads and isolated in-memory adapters."""
from __future__ import annotations

import hashlib
import io
import json
import sqlite3
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from pipelines.bitrix24.config import load_config
from pipelines.bitrix24.identity import entity_graph_uri
from pipelines.bitrix24.v2.protocol import canonical_json, sha256, validate_event, object_json
from pipelines.bitrix24.v2.runner import run_cycle_v2, apply_backfill_manifest
from pipelines.core.checkpoint import Checkpoint

ROOT = Path(__file__).resolve().parents[1]
ENTITIES = ("deal", "activity", "user", "category", "stage", "source", "lead")


class MemoryS3:
    def __init__(self, objects): self.objects, self.get_calls = objects, []
    def get_object(self, Bucket, Key):
        self.get_calls.append(Key)
        if Key not in self.objects: raise KeyError(Key)
        return {"Body": io.BytesIO(self.objects[Key])}


class MemoryRdf:
    def __init__(self, fail_after_write=False, fail_after_delete=False):
        self.graphs, self.calls = {}, []
        self.fail_after_write, self.fail_after_delete = fail_after_write, fail_after_delete
    def replace_graph(self, uri, data):
        self.graphs[uri] = data; self.calls.append(("PUT", uri))
        if self.fail_after_write:
            self.fail_after_write = False
            raise RuntimeError("injected RDF acknowledgement loss")
    def delete_graph(self, uri):
        self.graphs.pop(uri, None); self.calls.append(("DELETE", uri))
        if self.fail_after_delete:
            self.fail_after_delete = False
            raise RuntimeError("injected RDF delete acknowledgement loss")


def make_event(portal, entity, identity, title, stamp="2026-11-13T09:00:00Z"):
    if entity in {"stage", "source"}:
        a, b = identity.split(":", 1); payload = {"ENTITY_ID": a, "STATUS_ID": b, "NAME": title}
    else: payload = {"ID": identity, "TITLE": title}
    if stamp: payload["DATE_MODIFY"] = stamp
    digest = sha256(canonical_json({"event_id_algorithm": 3, "schema_version": 4,
                                    "payload": payload, "bindings": []}))
    token = stamp or "<null>"
    event_id = sha256(f"{portal}|{entity}|{identity}|{token}|{digest}".encode())
    return {"schema_version": 4, "event_id_algorithm": 3, "event_id": event_id,
            "graph_id": f"urn:bitrix24:graph:{portal}:{event_id}",
            "entity_key": f"{portal}/{entity}/{identity}", "operation": "upsert",
            "source_modified_at": stamp, "observed_at": stamp or "1970-01-01T00:00:00Z",
            "payload": payload, "bindings": []}


def serialize_event(portal, entity, payload):
    """Build a synthetic algorithm-3 event for loader contract tests."""
    identity = str(payload.get("ID", payload.get("STATUS_ID")))
    stamp = payload.get("DATE_MODIFY") or payload.get("LAST_UPDATED") or payload.get("MODIFIED")
    digest = sha256(canonical_json({"event_id_algorithm": 3, "schema_version": 4,
                                    "payload": payload, "bindings": []}))
    event_id = sha256(f"{portal}|{entity}|{identity}|{stamp or '<null>'}|{digest}".encode())
    return {"schema_version": 4, "event_id_algorithm": 3, "event_id": event_id,
            "graph_id": f"urn:bitrix24:graph:{portal}:{event_id}",
            "entity_key": f"{portal}/{entity}/{identity}", "operation": "upsert",
            "source_modified_at": stamp,
            "observed_at": stamp or "1970-01-01T00:00:00Z",
            "payload": payload, "bindings": []}


def put_event(objects, portal, value):
    _, entity, identity = value["entity_key"].split("/", 2)
    key = f"bitrix24/{portal}/events/{entity}/{identity}/{value['event_id']}.json"
    body = canonical_json(value); objects[key] = body
    return {"entity": entity, "event_id": value["event_id"], "graph_id": value["graph_id"],
            "key": key, "sha256": sha256(body)}


def counts(refs):
    result = dict.fromkeys(ENTITIES, 0)
    for ref in refs: result[ref["entity"]] += 1
    return result


def make_manifest(run_id, mode, epoch, refs=(), previous=None, previous_sha=None):
    value = {"contract_version": 2, "run_id": run_id, "mode": mode, "epoch_id": epoch,
             "previous_run_id": previous, "previous_manifest_sha256": previous_sha,
             "status": "complete", "counts": counts(refs), "objects": list(refs)}
    if mode == "local_snapshot":
        value.update({"complete": True, "complete_entity_types": ["deal", "category"],
                      "t0": "2026-11-13T08:00:00Z", "catchup_to": "2026-11-13T09:00:00Z",
                      "activity_from": "2026-11-13T07:00:00Z"})
    else: value["window"] = {"from": "2026-11-13T09:00:00Z", "to": "2026-11-13T10:00:00Z"}
    return value


def add_manifest(objects, value):
    key = f"bitrix24/portal-test/runs/{value['run_id']}/manifest.json"
    body = canonical_json(value); objects[key] = body
    return key, sha256(body)


def add_state(objects, epoch, base, base_sha, tip=None, tip_sha=None, revision=1):
    state = {"contract_version": 2, "revision": revision, "epoch_id": epoch,
             "base_manifest_key": base, "base_manifest_sha256": base_sha,
             "tip_run_id": tip, "tip_manifest_sha256": tip_sha}
    objects["bitrix24/portal-test/state.json"] = canonical_json(state)


def make_config(tmp, fast=False):
    Path(tmp).mkdir(parents=True, exist_ok=True)
    path = Path(tmp) / "config.yaml"
    path.write_text(f'''s3:\n  bucket: isolated\n  portal_id: portal-test\n  access_key: test\n  secret_key: test\nmapping:\n  registry_path: {ROOT / 'mappings/bitrix24/registry.json'}\n  schemas_root: {ROOT / 'mappings/bitrix24'}\ncheckpoint:\n  path: {Path(tmp) / 'state.sqlite3'}\n  resume_mode: {"fast" if fast else "strict"}\nbackend: fuseki\nfuseki:\n  url: http://127.0.0.1:3030/ds\ngraphdb:\n  url: http://127.0.0.1:7200\n  repository: test\nsnapshot_reconcile_entity_types: [deal, category]\n''', encoding="utf-8")
    return load_config(path)


class LoaderV2Tests(unittest.TestCase):
    def test_legacy_six_count_manifest_is_accepted_after_lead_addition(self):
        from pipelines.bitrix24.v2.protocol import validate_manifest
        manifest = make_manifest('20261113T120000Z-oldcounts', 'incremental',
                                 'local-20261113T080000Z-old', [])
        manifest['counts'].pop('lead')
        validate_manifest(manifest, 'bitrix24/portal-test/')

    def test_explicit_lead_backfill_verifies_sha_replays_and_never_rolls_back_newer(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            portal = 'portal-test'
            event = serialize_event(portal, 'lead', {'ID': '81', 'TITLE': 'synthetic lead',
                'DATE_CREATE': '2026-09-01T00:00:00Z', 'DATE_MODIFY': '2026-11-13T09:00:00Z'})
            ref = put_event(objects, portal, event)
            run_id = '20261113T120001Z-leadbackfill'
            manifest = make_manifest(run_id, 'backfill', 'local-20261113T080000Z-base', [ref])
            manifest['counts'] = {'lead': 1}
            key, digest = add_manifest(objects, manifest)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2('fuseki', 'isolated')
            rdf = MemoryRdf()
            try:
                with patch('pipelines.bitrix24.v2.runner.transform_event', return_value=b'<rdf/>'):
                    result = apply_backfill_manifest(MemoryS3(objects), cfg, cp, [], rdf, key, digest)
                    self.assertFalse(result['already_applied'])
                    self.assertEqual(len(rdf.calls), 1)
                    cp.save_entity('portal-test/lead/81', entity_graph_uri(cfg.graph_namespace, 'portal-test/lead/81'),
                                   'newer-event', '2026-11-13T10:00:00Z', False, 'newer-key', 'b'*64, 'map')
                    replay = apply_backfill_manifest(MemoryS3(objects), cfg, cp, [], rdf, key, digest)
                self.assertTrue(replay['already_applied'])
                self.assertEqual(len(rdf.calls), 1)
                self.assertEqual(cp.backfill_sha(key), digest)
                with self.assertRaisesRegex(ValueError, 'SHA-256 манифеста'):
                    apply_backfill_manifest(MemoryS3(objects), cfg, cp, [], rdf, key, 'f'*64)
            finally: cp.close()

    def test_lead_and_deal_with_same_id_have_distinct_entity_graphs(self):
        self.assertNotEqual(entity_graph_uri('urn:test', 'portal-test/deal/81'),
                            entity_graph_uri('urn:test', 'portal-test/lead/81'))

    def test_activity_bindings_are_preserved_in_mapping_xml(self):
        from pipelines.bitrix24.mapper import event_to_xml
        payload = {'ID': '7', 'OWNER_ID': '81', 'OWNER_TYPE_ID': '1'}
        xml = event_to_xml({'payload': payload, 'bindings': [
            {'entityTypeId': 1, 'entityId': ' 81 '}, {'entityTypeId': 2, 'entityId': 81}]})
        self.assertIn('<entityTypeId>1</entityTypeId>', xml)
        self.assertIn('<entityTypeId>2</entityTypeId>', xml)
        self.assertIn('<entityId>81</entityId>', xml)

    def test_independent_exporter_control_vectors(self):
        fixture = ROOT / "tests/bitrix24/v2/fixtures/exporter_control_vectors_copy.json"
        for vector in json.loads(fixture.read_text(encoding="utf-8"))["vectors"]:
            body = vector["canonical_event"].encode("utf-8")
            self.assertEqual(sha256(body), vector["sha256"])
            value = object_json(body, vector["key"])
            ref = {"key": vector["key"], "event_id": vector["event_id"],
                   "entity": vector["entity"], "graph_id": value["graph_id"]}
            self.assertEqual(validate_event(value, ref, vector["portal"]), value["entity_key"])

    def test_legacy_algorithm3_vector_remains_valid(self):
        event = {"schema_version": 4, "event_id_algorithm": 3,
                 "event_id": "50ce4ce05f22d26db90ccf47ee00367167f2a5abcea5164730661c43c0abde8c",
                 "graph_id": "urn:bitrix24:graph:garpix:50ce4ce05f22d26db90ccf47ee00367167f2a5abcea5164730661c43c0abde8c",
                 "entity_key": "garpix/stage/DEAL_STAGE_2:NEW", "operation": "upsert",
                 "source_modified_at": None, "observed_at": "1970-01-01T00:00:00Z",
                 "payload": {"ENTITY_ID":"DEAL_STAGE_2","STATUS_ID":"NEW","NAME":"Новая"},
                 "bindings": []}
        ref = {"entity":"stage", "event_id":event["event_id"], "graph_id":event["graph_id"],
               "key":"bitrix24/garpix/events/stage/DEAL_STAGE_2:NEW/" + event["event_id"] + ".json"}
        self.assertEqual(validate_event(event, ref, "garpix"), event["entity_key"])

    def test_empty_snapshot_establishes_epoch_without_graphs(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            snap = make_manifest("local-20261113T080000Z-empty", "local_snapshot",
                                 "local-20261113T080000Z-empty", [])
            base, digest = add_manifest(objects, snap)
            add_state(objects, snap["epoch_id"], base, digest)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            try:
                run_cycle_v2(MemoryS3(objects), cfg, cp, [], MemoryRdf())
                self.assertEqual(cp.get_meta("epoch_id"), snap["epoch_id"])
                self.assertEqual(cp.get_meta("base_manifest_sha256"), digest)
                self.assertEqual(cp.active_entities(), [])
                self.assertEqual(cp.get_meta("applied_tip_run_id"), "")
            finally: cp.close()

    def test_state_revision_cannot_regress_behind_checkpoint(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            snap = make_manifest("local-20261113T080000Z-revision", "local_snapshot",
                                 "local-20261113T080000Z-revision", [])
            base, digest = add_manifest(objects, snap)
            add_state(objects, snap["epoch_id"], base, digest, revision=4)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            try:
                run_cycle_v2(MemoryS3(objects), cfg, cp, [], MemoryRdf())
                add_state(objects, snap["epoch_id"], base, digest, revision=3)
                with self.assertRaisesRegex(RuntimeError, "revision отстаёт"):
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], MemoryRdf())
                self.assertEqual(cp.get_meta("observed_state_revision"), "4")
            finally: cp.close()

    def test_fuseki_and_graphdb_adapters_receive_expected_graph_uri(self):
        from pipelines.stores.fuseki import FusekiStore
        from pipelines.stores.graphdb import GraphDBStore
        for backend, store, expected_endpoint in (
            ("fuseki", FusekiStore("http://fuseki.invalid/ds"), "http://fuseki.invalid/ds/data"),
            ("graphdb", GraphDBStore("http://graphdb.invalid:7200", "repo-v2"),
             "http://graphdb.invalid:7200/repositories/repo-v2/rdf-graphs/service"),
        ):
            with self.subTest(backend=backend), tempfile.TemporaryDirectory() as tmp:
                cfg, objects = make_config(tmp), {}
                value = make_event("portal-test", "deal", "88", "synthetic")
                ref = put_event(objects, "portal-test", value)
                snap = make_manifest("local-20261113T080000Z-adapter", "local_snapshot",
                                     "local-20261113T080000Z-adapter", [ref])
                base, digest = add_manifest(objects, snap); add_state(objects, snap["epoch_id"], base, digest)
                cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2(backend, expected_endpoint)
                captured = []
                response = type("Response", (), {"status_code": 204, "text": ""})()
                try:
                    with patch("pipelines.bitrix24.v2.runner.transform_event", return_value=b"synthetic RDF"), \
                         patch("pipelines.stores._http.requests.request", side_effect=lambda method, endpoint, **kw: captured.append((method, endpoint, kw)) or response):
                        run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(len(captured), 1)
                    self.assertEqual(captured[0][0], "PUT")
                    self.assertEqual(captured[0][1], expected_endpoint)
                    self.assertEqual(captured[0][2]["params"]["graph"],
                                     entity_graph_uri(cfg.graph_namespace, value["entity_key"]))
                    self.assertEqual(cp.get_meta("applied_tip_run_id"), "")
                finally: cp.close()

    def test_snapshot_incremental_empty_repeat_and_epoch(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            first = make_event("portal-test", "deal", "42", "initial")
            second = make_event("portal-test", "deal", "42", "incremental")
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot",
                                 "local-20261113T080000Z-epoch1", [put_event(objects, "portal-test", first)])
            base, base_sha = add_manifest(objects, snap)
            inc = make_manifest("20261113T100000Z-run1", "incremental", snap["epoch_id"])
            inc["objects"] = [put_event(objects, "portal-test", second)]; inc["counts"] = counts(inc["objects"])
            inc_key, inc_sha = add_manifest(objects, inc)
            empty = make_manifest("20261113T110000Z-run2", "incremental", snap["epoch_id"], [], inc["run_id"], inc_sha)
            empty["window"] = {"from": "2026-11-13T10:00:00Z", "to": "2026-11-13T11:00:00Z"}
            _, empty_sha = add_manifest(objects, empty)
            add_state(objects, snap["epoch_id"], base, base_sha, empty["run_id"], empty_sha, 2)
            objects[inc_key[:-len("manifest.json")] + "status.json"] = b'{"status":"failed"}'
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            store = MemoryRdf()
            try:
                with patch("pipelines.bitrix24.v2.runner.transform_event",
                           side_effect=lambda _c, _m, e, _k: canonical_json(e["payload"])):
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    uri = entity_graph_uri(cfg.graph_namespace, first["entity_key"])
                    self.assertEqual(json.loads(store.graphs[uri]), second["payload"])
                    self.assertEqual(cp.get_meta("applied_tip_run_id"), empty["run_id"])
                    self.assertEqual(cp.get_meta("applied_tip_manifest_sha256"), empty_sha)
                    old_graphs = dict(store.graphs)
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(store.graphs, old_graphs)
                    fresh = make_event("portal-test", "deal", "42", "new epoch")
                    newer = make_manifest("local-20261113T120000Z-epoch2", "local_snapshot",
                                          "local-20261113T120000Z-epoch2", [put_event(objects, "portal-test", fresh)])
                    newer_key, newer_sha = add_manifest(objects, newer)
                    add_state(objects, newer["epoch_id"], newer_key, newer_sha, revision=3)
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(json.loads(store.graphs[uri]), fresh["payload"])
                    self.assertEqual(cp.get_meta("epoch_id"), newer["epoch_id"])
                    self.assertEqual(cp.get_meta("applied_tip_run_id"), "")
            finally: cp.close()

    def test_two_independent_checkpoints_converge_from_different_lag(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg_a = make_config(Path(tmp) / "a")
            cfg_b = make_config(Path(tmp) / "b")
            objects = {}
            initial = make_event("portal-test", "deal", "200", "snapshot")
            first = make_event("portal-test", "deal", "200", "incremental one")
            second = make_event("portal-test", "deal", "200", "incremental two")
            snap = make_manifest("local-20261113T080000Z-independent", "local_snapshot",
                                 "local-20261113T080000Z-independent",
                                 [put_event(objects, "portal-test", initial)])
            base, base_sha = add_manifest(objects, snap)
            run1 = make_manifest("20261113T090000Z-independent1", "incremental", snap["epoch_id"])
            run1["objects"] = [put_event(objects, "portal-test", first)]; run1["counts"] = counts(run1["objects"])
            _, run1_sha = add_manifest(objects, run1)
            run2 = make_manifest("20261113T100000Z-independent2", "incremental", snap["epoch_id"],
                                 [], run1["run_id"], run1_sha)
            run2["objects"] = [put_event(objects, "portal-test", second)]; run2["counts"] = counts(run2["objects"])
            _, run2_sha = add_manifest(objects, run2)
            state_key = "bitrix24/portal-test/state.json"
            add_state(objects, snap["epoch_id"], base, base_sha, run1["run_id"], run1_sha, 2)
            cp_a = Checkpoint(cfg_a.checkpoint_path); cp_a.bind_store_v2("fuseki", "isolated-a")
            store_a = MemoryRdf()
            cp_b = Checkpoint(cfg_b.checkpoint_path); cp_b.bind_store_v2("graphdb", "isolated-b")
            store_b = MemoryRdf()
            try:
                with patch("pipelines.bitrix24.v2.runner.transform_event",
                           side_effect=lambda _c, _m, e, _k: canonical_json(e["payload"])):
                    run_cycle_v2(MemoryS3(objects), cfg_a, cp_a, [], store_a)
                    add_state(objects, snap["epoch_id"], base, base_sha, run2["run_id"], run2_sha, 3)
                    run_cycle_v2(MemoryS3(objects), cfg_b, cp_b, [], store_b)
                    run_cycle_v2(MemoryS3(objects), cfg_a, cp_a, [], store_a)
                self.assertEqual(store_a.graphs, store_b.graphs)
                self.assertEqual(cp_a.get_meta("applied_tip_run_id"), run2["run_id"])
                self.assertEqual(cp_b.get_meta("applied_tip_run_id"), run2["run_id"])
                self.assertEqual(cp_a.get_meta("applied_tip_manifest_sha256"), run2_sha)
                self.assertEqual(cp_b.get_meta("applied_tip_manifest_sha256"), run2_sha)
                self.assertIn(state_key, objects)
            finally:
                cp_a.close(); cp_b.close()

    def test_category_112_tampering_is_rejected_before_rdf(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            category = make_event("portal-test", "category", "112", "synthetic category")
            ref = put_event(objects, "portal-test", category)
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot",
                                 "local-20261113T080000Z-epoch1", [ref])
            base, digest = add_manifest(objects, snap); add_state(objects, snap["epoch_id"], base, digest)
            objects[ref["key"]] = objects[ref["key"]].replace(b"synthetic category", b"tampered category")
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            store = MemoryRdf()
            try:
                with patch("pipelines.bitrix24.v2.runner.transform_event", return_value=b"synthetic RDF"):
                    with self.assertRaisesRegex(ValueError, "SHA-256"):
                        run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                self.assertEqual(store.graphs, {})
                self.assertIsNone(cp.get_meta("base_manifest_sha256"))
                self.assertEqual(cp.active_entities(), [])
            finally: cp.close()

    def test_rdf_ack_loss_retries_before_checkpoint(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            value = make_event("portal-test", "deal", "12", "synthetic")
            ref = put_event(objects, "portal-test", value)
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot",
                                 "local-20261113T080000Z-epoch1", [ref])
            base, digest = add_manifest(objects, snap); add_state(objects, snap["epoch_id"], base, digest)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            store = MemoryRdf(fail_after_write=True)
            try:
                with patch("pipelines.bitrix24.v2.runner.transform_event", return_value=b"synthetic RDF"):
                    with self.assertRaisesRegex(RuntimeError, "RDF PUT"):
                        run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(cp.active_entities(), [])
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(len(cp.active_entities()), 1)
                    self.assertEqual(len(store.calls), 2)
            finally: cp.close()

    def test_restart_when_sqlite_entity_write_fails_after_rdf_put(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            value = make_event("portal-test", "deal", "13", "synthetic")
            ref = put_event(objects, "portal-test", value)
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot",
                                 "local-20261113T080000Z-epoch1", [ref])
            base, digest = add_manifest(objects, snap); add_state(objects, snap["epoch_id"], base, digest)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            store, original, count = MemoryRdf(), cp.save_entity, [0]
            def fail_once(*args, **kwargs):
                count[0] += 1
                if count[0] == 1: raise RuntimeError("injected SQLite entity failure")
                return original(*args, **kwargs)
            try:
                with patch("pipelines.bitrix24.v2.runner.transform_event", return_value=b"synthetic RDF"), \
                     patch.object(cp, "save_entity", side_effect=fail_once):
                    with self.assertRaisesRegex(RuntimeError, "checkpoint write"):
                        run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(cp.active_entities(), [])
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(len(cp.active_entities()), 1)
                    self.assertEqual(len(store.calls), 2)
            finally: cp.close()

    def test_restart_after_snapshot_reconcile_delete_and_before_run_finish(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            old_key = "portal-test/category/112"
            old_uri = entity_graph_uri(cfg.graph_namespace, old_key)
            cp.save_entity(old_key, old_uri, "old-event", None, False)
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot",
                                 "local-20261113T080000Z-epoch1", [])
            base, digest = add_manifest(objects, snap); add_state(objects, snap["epoch_id"], base, digest)
            store = MemoryRdf(fail_after_delete=True)
            original_finish, finish_calls = cp.finish_v2_run, [0]
            def fail_finish_once(*args, **kwargs):
                finish_calls[0] += 1
                if finish_calls[0] == 1: raise RuntimeError("injected run checkpoint failure")
                return original_finish(*args, **kwargs)
            try:
                with patch.object(cp, "finish_v2_run", side_effect=fail_finish_once):
                    with self.assertRaisesRegex(RuntimeError, "Snapshot reconciliation"):
                        run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(cp.get_entity(old_key)["deleted"], 0)
                    with self.assertRaisesRegex(RuntimeError, "run checkpoint"):
                        run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertEqual(cp.get_entity(old_key)["deleted"], 1)
                    self.assertFalse(cp.run_done(snap["run_id"]))
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], store)
                    self.assertTrue(cp.run_done(snap["run_id"]))
                    self.assertEqual([op for op, _ in store.calls].count("DELETE"), 2)
            finally: cp.close()

    def test_missing_predecessor_and_nonancestor_checkpoint_are_blocked(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot", "local-20261113T080000Z-epoch1")
            base, base_sha = add_manifest(objects, snap)
            tip = make_manifest("20261113T100000Z-run2", "incremental", snap["epoch_id"], [],
                                "20261113T090000Z-run1", "0" * 64)
            _, tip_sha = add_manifest(objects, tip)
            add_state(objects, snap["epoch_id"], base, base_sha, tip["run_id"], tip_sha, 2)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            try:
                with self.assertRaises(KeyError): run_cycle_v2(MemoryS3(objects), cfg, cp, [], MemoryRdf())
                self.assertEqual(cp.get_meta("applied_tip_run_id"), "")
                tip["previous_run_id"] = None
                tip["previous_manifest_sha256"] = None
                tip_key = f"bitrix24/portal-test/runs/{tip['run_id']}/manifest.json"
                objects[tip_key] = canonical_json(tip)
                add_state(objects, snap["epoch_id"], base, base_sha, tip["run_id"], sha256(objects[tip_key]), 3)
                cp.set_meta("applied_tip_run_id", "20261113T090000Z-orphan")
                cp.set_meta("applied_tip_manifest_sha256", "1" * 64)
                with self.assertRaisesRegex(RuntimeError, "не является предком"):
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], MemoryRdf())
            finally: cp.close()

    def test_fast_resume_blind_spot_is_limited_to_exact_checkpointed_ref(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp, fast=True), {}
            value = make_event("portal-test", "deal", "14", "synthetic")
            ref = put_event(objects, "portal-test", value)
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot",
                                 "local-20261113T080000Z-epoch1", [ref])
            base, digest = add_manifest(objects, snap); add_state(objects, snap["epoch_id"], base, digest)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            store, source = MemoryRdf(), MemoryS3(objects)
            try:
                with patch("pipelines.bitrix24.v2.runner.transform_event", return_value=b"synthetic RDF"):
                    run_cycle_v2(source, cfg, cp, [], store)
                    source.get_calls.clear()
                    objects[ref["key"]] = objects[ref["key"]].replace(b"synthetic", b"tampered")
                    run_cycle_v2(source, cfg, cp, [], store)
                    self.assertNotIn(ref["key"], source.get_calls)
                    self.assertIn("bitrix24/portal-test/state.json", source.get_calls)
            finally: cp.close()

    def test_strict_repeat_detects_snapshot_object_tampering(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp, fast=False), {}
            value = make_event("portal-test", "deal", "15", "synthetic")
            ref = put_event(objects, "portal-test", value)
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot",
                                 "local-20261113T080000Z-epoch1", [ref])
            base, digest = add_manifest(objects, snap); add_state(objects, snap["epoch_id"], base, digest)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            store, source = MemoryRdf(), MemoryS3(objects)
            try:
                with patch("pipelines.bitrix24.v2.runner.transform_event", return_value=b"synthetic RDF"):
                    run_cycle_v2(source, cfg, cp, [], store)
                    objects[ref["key"]] = objects[ref["key"]].replace(b"synthetic", b"tampered")
                    with self.assertRaisesRegex(ValueError, "SHA-256"):
                        run_cycle_v2(source, cfg, cp, [], store)
                    self.assertEqual(len(store.calls), 1)
                    self.assertEqual(len(cp.active_entities()), 1)
            finally: cp.close()

    def test_tampered_confirmed_manifest_sha_stops_before_run_promotion(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg, objects = make_config(tmp), {}
            snap = make_manifest("local-20261113T080000Z-epoch1", "local_snapshot", "local-20261113T080000Z-epoch1")
            base, base_sha = add_manifest(objects, snap)
            run = make_manifest("20261113T100000Z-run1", "incremental", snap["epoch_id"])
            run_key, _ = add_manifest(objects, run)
            add_state(objects, snap["epoch_id"], base, base_sha, run["run_id"], "0" * 64, 2)
            cp = Checkpoint(cfg.checkpoint_path); cp.bind_store_v2("fuseki", "isolated")
            try:
                with self.assertRaisesRegex(ValueError, "SHA-256"):
                    run_cycle_v2(MemoryS3(objects), cfg, cp, [], MemoryRdf())
                self.assertEqual(cp.get_meta("applied_tip_run_id"), "")
                self.assertEqual(cp.get_meta("base_manifest_sha256"), base_sha)
            finally: cp.close()

    def test_legacy_checkpoint_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg = make_config(tmp)
            with sqlite3.connect(cfg.checkpoint_path) as db:
                db.execute("CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT NOT NULL)")
                db.execute("INSERT INTO meta VALUES ('checkpoint_version', '1')")
            with self.assertRaisesRegex(RuntimeError, "только v2 checkpoint"):
                Checkpoint(cfg.checkpoint_path)


if __name__ == "__main__": unittest.main()
