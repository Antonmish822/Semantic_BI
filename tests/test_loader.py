import hashlib
import io
import importlib.util
import xml.etree.ElementTree as ET
import json
import sqlite3
import requests
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from pipelines.bitrix24.config import load_config
from pipelines.bitrix24.identity import entity_graph_uri
from pipelines.bitrix24.mapper import load_mappings, transform_event
from pipelines.bitrix24.source import canonical_json, validate_event, validate_manifest, read_base_snapshot
from pipelines.core.checkpoint import Checkpoint
from pipelines.bitrix24.runner import apply_event, process_manifest
from pipelines.stores.fuseki import FusekiStore
from pipelines.stores.graphdb import GraphDBStore

ROOT = Path(__file__).resolve().parents[1]


def config_text(backend='fuseki', checkpoint='state.sqlite3', resume_mode='strict'):
    return f'''s3:
  bucket: test
  portal_id: portal-a
  access_key: test
  secret_key: test
mapping:
  registry_path: {ROOT / 'mappings/bitrix24/registry.json'}
  schemas_root: {ROOT / 'mappings/bitrix24'}
checkpoint:
  path: {checkpoint}
  resume_mode: {resume_mode}
backend: {backend}
fuseki:
  url: http://fuseki.invalid/ds
  graph_namespace: urn:bitrix24:entity
graphdb:
  url: http://graphdb.invalid:7200
  repository: repo-a
  graph_namespace: urn:bitrix24:entity
'''


def load_test_config(tmp, backend='fuseki', resume_mode='strict'):
    p = Path(tmp) / 'config.yaml'
    p.write_text(config_text(backend, resume_mode=resume_mode), encoding='utf-8')
    return load_config(p)


class FakeS3:
    def __init__(self, objects): self.objects = objects; self.get_calls=[]
    def get_object(self, Bucket, Key):
        self.get_calls.append(Key)
        return {'Body': io.BytesIO(self.objects[Key])}


class FakeStore:
    def __init__(self, fail=False): self.calls=[]; self.fail=fail
    def replace_graph(self, uri, data):
        if self.fail: raise RuntimeError('store failed')
        self.calls.append(('PUT', uri, data))
    def delete_graph(self, uri):
        if self.fail: raise RuntimeError('store failed')
        self.calls.append(('DELETE', uri))


def upsert(title='first', identity='42'):
    payload={'ID':str(identity),'TITLE':title,'DATE_MODIFY':'2026-01-01T00:00:00Z'}
    digest=hashlib.sha256(canonical_json({'payload':payload,'bindings':[]})).hexdigest()
    event_id=hashlib.sha256(f"portal-a|deal|{identity}|{payload['DATE_MODIFY']}|{digest}".encode()).hexdigest()
    return {'schema_version':3,'event_id':event_id,'graph_id':f'urn:bitrix24:graph:portal-a:{event_id}',
            'entity_key':f'portal-a/deal/{identity}','operation':'upsert','source_modified_at':payload['DATE_MODIFY'],
            'payload':payload,'bindings':[]}


def ref_for(event):
    body=json.dumps(event).encode()
    portal,entity_type,identity=event['entity_key'].split('/',2)
    key=f"bitrix24/{portal}/events/{entity_type}/{identity}/{event['event_id']}.json"
    return {'key':key,'event_id':event['event_id'],'graph_id':event['graph_id'],
            'sha256':hashlib.sha256(body).hexdigest(),'entity':entity_type},body


class ConfigTests(unittest.TestCase):
    def test_explicit_backend_and_mappings(self):
        with tempfile.TemporaryDirectory() as tmp:
            for backend in ('fuseki','graphdb'):
                cfg=load_test_config(tmp,backend)
                self.assertEqual(cfg.backend,backend)
                self.assertEqual(len(load_mappings(cfg)),6)
                self.assertEqual(cfg.graph_namespace,'urn:bitrix24:entity')
            p=Path(tmp)/'config.yaml'
            p.write_text(config_text('unknown'))
            with self.assertRaisesRegex(ValueError,'backend'):
                load_config(p)
            p.write_text(config_text('graphdb').replace('  repository: repo-a\n',''))
            with self.assertRaisesRegex(ValueError,'repository'):
                load_config(p)

    @unittest.skipUnless(importlib.util.find_spec('cccr_py'), 'native CCCR unavailable')
    def test_native_snapshot_fixtures_transform(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg=load_test_config(tmp)
            mappings=load_mappings(cfg)
            fixtures=ROOT/'tests/bitrix24/fixtures/snapshot-2026-11-04'
            expected=json.loads((fixtures/'expected.json').read_text())
            for entity_type in ('deal','activity','category','stage','source','user'):
                with self.subTest(entity=entity_type):
                    payload=json.loads((fixtures/f'{entity_type}.json').read_text())
                    identity = ('DEAL_STAGE_901:C901:NEW' if entity_type=='stage' else
                                'SOURCE:2|FIXTURE_SOURCE' if entity_type=='source' else payload['ID'])
                    rdf=transform_event(cfg,mappings,{'payload':payload},f'{entity_type}/{identity}/event.json')
                    graph=ET.fromstring(rdf)
                    wanted=expected[entity_type]
                    ns='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
                    self.assertTrue(any(
                        desc.get(f'{{{ns}}}about')==wanted['subject'] and
                        any(child.tag.rsplit('}',1)[-1]==wanted['predicate'] and (child.text or '')==wanted['value']
                            for child in desc)
                        for desc in graph.findall(f'{{{ns}}}Description')))

    def test_uri_matches_legacy_namespace(self):
        key='portal-a/deal/42'
        expected='urn:bitrix24:entity:portal-a:deal:'+hashlib.sha256(key.encode()).hexdigest()
        self.assertEqual(entity_graph_uri('urn:bitrix24:entity',key),expected)


class StoreContract(unittest.TestCase):
    def test_put_delete_retries_and_errors_for_both_adapters(self):
        for store,endpoint in [
            (FusekiStore('http://fuseki.invalid/ds','u','p',7),'http://fuseki.invalid/ds/data'),
            (GraphDBStore('http://graphdb.invalid:7200','repo-a','u','p',7),
             'http://graphdb.invalid:7200/repositories/repo-a/rdf-graphs/service')]:
            with self.subTest(store=store.__class__.__name__):
                calls=[]
                def request(method,url,**kw):
                    calls.append((method,url,kw))
                    return type('Response',(),{'status_code':204 if method=='PUT' else 404,'text':''})()
                with patch('pipelines.stores._http.requests.request',side_effect=request):
                    for _ in range(2):
                        store.replace_graph('urn:g:1',b'<rdf/>')
                        store.delete_graph('urn:g:1')
                self.assertEqual([c[0] for c in calls],['PUT','DELETE','PUT','DELETE'])
                self.assertTrue(all(c[1]==endpoint and c[2]['params']=={'graph':'urn:g:1'} for c in calls))
                self.assertEqual(calls[0][2]['headers']['Content-Type'],'application/rdf+xml; charset=utf-8')
                self.assertEqual(calls[0][2]['timeout'],7)
                self.assertEqual(calls[0][2]['auth'],('u','p'))
                with patch('pipelines.stores._http.requests.request',side_effect=requests.Timeout('timed out')):
                    with self.assertRaisesRegex(RuntimeError,'timed out'):
                        store.replace_graph('urn:g:1',b'<rdf/>')
                with patch('pipelines.stores._http.requests.request',return_value=type('R',(),{'status_code':500,'text':'failure'})()):
                    with self.assertRaisesRegex(RuntimeError,'HTTP 500'):
                        store.replace_graph('urn:g:1',b'<rdf/>')
                    with self.assertRaisesRegex(RuntimeError,'HTTP 500'):
                        store.delete_graph('urn:g:1')


class PipelineTests(unittest.TestCase):
    def test_checkpoint_legacy_fuseki_and_graphdb_isolation(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'state.sqlite3'
            cp=Checkpoint(path)
            cp.save_entity('portal-a/deal/42','urn:g:1','e1',None,False)
            cp.close()
            cp=Checkpoint(path)
            try:
                with self.assertRaisesRegex(RuntimeError,'Fuseki'):
                    cp.bind_store('graphdb','http://graphdb')
                cp.bind_store('fuseki','http://fuseki/ds/data')
                with self.assertRaisesRegex(RuntimeError,'привязан'):
                    cp.bind_store('graphdb','http://graphdb')
            finally: cp.close()
            cp=Checkpoint(Path(tmp)/'graphdb.sqlite3')
            try:
                cp.bind_store('graphdb','http://graphdb')
                self.assertEqual(cp.get_meta('store_backend'),'graphdb')
            finally: cp.close()

    def test_unknown_checkpoint_version_fails_without_migration(self):
        with tempfile.TemporaryDirectory() as tmp:
            path=Path(tmp)/'state.sqlite3'
            with sqlite3.connect(path) as db:
                db.execute('CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT NOT NULL)')
                db.execute("INSERT INTO meta VALUES ('checkpoint_version','999')")
            with self.assertRaisesRegex(RuntimeError,'Неизвестная версия'):
                Checkpoint(path)
            with sqlite3.connect(path) as db:
                self.assertEqual(db.execute('SELECT value FROM meta WHERE key=?',('checkpoint_version',)).fetchone()[0],'999')
                self.assertIsNone(db.execute("SELECT name FROM sqlite_master WHERE name='entities'").fetchone())

    def test_failed_put_delete_do_not_advance_checkpoint(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg=load_test_config(tmp)
            cp=Checkpoint(cfg.checkpoint_path)
            try:
                event=upsert();ref,body=ref_for(event);source=FakeS3({ref['key']:body})
                with patch('pipelines.bitrix24.runner.transform_event',return_value=b'<rdf/>'):
                    with self.assertRaisesRegex(RuntimeError,'store failed'):
                        apply_event(source,cfg,cp,[],ref,'mapping-v1',store=FakeStore(True))
                    self.assertIsNone(cp.get_entity(event['entity_key']))
                    store=FakeStore()
                    apply_event(source,cfg,cp,[],ref,'mapping-v1',store=store)
                    apply_event(source,cfg,cp,[],ref,'mapping-v1',store=store)
                    self.assertEqual(len(store.calls),1)
                deleted={'schema_version':3,'event_id':'delete-1','graph_id':'urn:bitrix24:graph:portal-a:delete-1',
                         'entity_key':event['entity_key'],'operation':'delete','source_modified_at':'2026-01-02T00:00:00Z'}
                delete_ref,delete_body=ref_for(deleted)
                source.objects[delete_ref['key']]=delete_body
                with self.assertRaisesRegex(RuntimeError,'store failed'):
                    apply_event(source,cfg,cp,[],delete_ref,'mapping-v1',store=FakeStore(True))
                self.assertEqual(cp.get_entity(event['entity_key'])['deleted'],0)
                apply_event(source,cfg,cp,[],delete_ref,'mapping-v1',store=store)
                self.assertEqual(cp.get_entity(event['entity_key'])['deleted'],1)
                self.assertEqual(store.calls[-1],('DELETE',entity_graph_uri(cfg.graph_namespace,event['entity_key'])))
            finally: cp.close()

    def test_manifest_event_and_sha_validation(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg=load_test_config(tmp);cp=Checkpoint(cfg.checkpoint_path)
            try:
                event=upsert();ref,body=ref_for(event)
                self.assertEqual(validate_event(event,ref,'portal-a','deal','42'),event['entity_key'])
                with self.assertRaisesRegex(ValueError,'event_id'):
                    validate_event({**event,'payload':{**event['payload'],'TITLE':'tampered'}},ref,'portal-a','deal','42')
                with self.assertRaisesRegex(ValueError,'SHA-256'):
                    apply_event(FakeS3({ref['key']:body}),cfg,cp,[],{**ref,'sha256':'0'*64},'v',store=FakeStore())
                self.assertIsNone(cp.get_entity(event['entity_key']))
                manifest={'run_id':'r1','mode':'local_snapshot','status':'complete','complete':True,
                          'counts':{'deal':1},'objects':[ref]}
                self.assertEqual(validate_manifest(manifest,'r1','local_snapshot'),[ref])
                with self.assertRaisesRegex(ValueError,'Counts'):
                    validate_manifest({**manifest,'counts':{}},'r1','local_snapshot')
            finally: cp.close()

    def test_snapshot_reconciles_only_configured_types(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg=load_test_config(tmp);cp=Checkpoint(cfg.checkpoint_path)
            old_uri=entity_graph_uri(cfg.graph_namespace,'portal-a/deal/41')
            activity_uri=entity_graph_uri(cfg.graph_namespace,'portal-a/activity/7')
            cp.save_entity('portal-a/deal/41',old_uri,'old',None,False)
            cp.save_entity('portal-a/activity/7',activity_uri,'old',None,False)
            event=upsert();ref,body=ref_for(event)
            manifest={'run_id':'local-r1','mode':'local_snapshot','status':'complete','complete':True,
                      'catchup_to':'2026-01-01T00:00:00Z','counts':{'deal':1},'objects':[ref]}
            store=FakeStore()
            try:
                with patch('pipelines.bitrix24.runner.transform_event',return_value=b'<rdf/>'):
                    process_manifest(FakeS3({ref['key']:body}),cfg,cp,[],'runs/local-r1/manifest.json',
                                     manifest,'local_snapshot','v1',store=store)
                self.assertIn(('DELETE',old_uri),store.calls)
                self.assertNotIn(('DELETE',activity_uri),store.calls)
                self.assertEqual(cp.get_entity('portal-a/deal/41')['deleted'],1)
                self.assertEqual(cp.get_entity('portal-a/activity/7')['deleted'],0)
                self.assertTrue(cp.run_done('local-r1'))
            finally: cp.close()

    def test_fast_resume_skips_download_of_checkpointed_snapshot_events(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg=load_test_config(tmp,resume_mode='fast');cp=Checkpoint(cfg.checkpoint_path)
            events=[upsert('first','42'),upsert('second','43')]
            refs_and_bodies=[ref_for(event) for event in events]
            refs=[item[0] for item in refs_and_bodies]
            source=FakeS3({ref['key']:body for ref,body in refs_and_bodies})
            manifest={'run_id':'local-r-fast','mode':'local_snapshot','status':'complete','complete':True,
                      'catchup_to':'2026-01-01T00:00:00Z','counts':{'deal':2},'objects':refs}

            class InterruptOnceStore(FakeStore):
                def __init__(self): super().__init__(); self.interrupted=False
                def replace_graph(self, uri, data):
                    if len(self.calls)==1 and not self.interrupted:
                        self.interrupted=True
                        raise RuntimeError('simulated interruption')
                    super().replace_graph(uri,data)

            store=InterruptOnceStore()
            try:
                with patch('pipelines.bitrix24.runner.transform_event',return_value=b'<rdf/>'):
                    with self.assertRaisesRegex(RuntimeError,'simulated interruption'):
                        process_manifest(source,cfg,cp,[],'runs/local-r-fast/manifest.json',
                                         manifest,'local_snapshot','mapping-v1',store=store)
                    self.assertIsNotNone(cp.get_entity(events[0]['entity_key']))
                    self.assertIsNone(cp.get_entity(events[1]['entity_key']))
                    self.assertFalse(cp.run_done('local-r-fast'))
                    process_manifest(source,cfg,cp,[],'runs/local-r-fast/manifest.json',
                                     manifest,'local_snapshot','mapping-v1',store=store)
                self.assertEqual(source.get_calls,[refs[0]['key'],refs[1]['key'],refs[1]['key']])
                self.assertTrue(cp.run_done('local-r-fast'))
                self.assertEqual(len(store.calls),2)
            finally: cp.close()

    def test_failed_manifest_replays_without_marking_run_done(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg=load_test_config(tmp);cp=Checkpoint(cfg.checkpoint_path)
            event=upsert();ref,body=ref_for(event)
            manifest={'run_id':'r1','mode':'incremental','status':'complete',
                      'window':{'from':'2025-12-31T00:00:00Z','to':'2026-01-02T00:00:00Z'},
                      'objects':[{**ref,'sha256':'f'*64}]}
            store=FakeStore();source=FakeS3({ref['key']:body})
            try:
                with patch('pipelines.bitrix24.runner.transform_event',return_value=b'<rdf/>'):
                    with self.assertRaisesRegex(ValueError,'SHA-256'):
                        process_manifest(source,cfg,cp,[],'runs/r1/manifest.json',manifest,'incremental','v1',store=store)
                    self.assertFalse(cp.run_done('r1'))
                    manifest['objects']=[ref]
                    process_manifest(source,cfg,cp,[],'runs/r1/manifest.json',manifest,'incremental','v1',store=store)
                    process_manifest(source,cfg,cp,[],'runs/r1/manifest.json',manifest,'incremental','v1',store=store)
                self.assertTrue(cp.run_done('r1'))
                self.assertEqual(len(store.calls),1)
            finally: cp.close()

    def test_snapshot_manifest_sha_is_checked(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg=load_test_config(tmp)
            manifest={'run_id':'local-r1','mode':'local_snapshot','status':'complete','complete':True,
                      'counts':{'deal':1},'objects':[{'key':'x','entity':'deal'}]}
            key=cfg.runs_prefix+'local-r1/manifest.json'
            s3=FakeS3({key:json.dumps(manifest).encode()})
            state={'adopted_local_manifest_key':key,
                   'adopted_local_manifest_sha256':hashlib.sha256(canonical_json(manifest)).hexdigest()}
            self.assertEqual(read_base_snapshot(s3,cfg,state)[1],manifest)
            with self.assertRaisesRegex(ValueError,'SHA-256'):
                read_base_snapshot(s3,cfg,{**state,'adopted_local_manifest_sha256':'0'*64})

if __name__=='__main__': unittest.main()
