import hashlib
import importlib.util
import json
import requests
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from unittest.mock import patch

from pipelines.bitrix24.config import load_config
from pipelines.bitrix24.identity import entity_graph_uri
from pipelines.bitrix24.mapper import load_mappings, transform_event
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


class ConfigTests(unittest.TestCase):
    def test_explicit_backend_and_mappings(self):
        with tempfile.TemporaryDirectory() as tmp:
            for backend in ('fuseki','graphdb'):
                cfg=load_test_config(tmp,backend)
                self.assertEqual(cfg.backend,backend)
                self.assertEqual(len(load_mappings(cfg)),7)
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

    @unittest.skipUnless(importlib.util.find_spec('cccr_py'), 'native CCCR unavailable')
    def test_synthetic_lead_deal_and_activity_mappings(self):
        with tempfile.TemporaryDirectory() as tmp:
            cfg=load_test_config(tmp)
            mappings=load_mappings(cfg)

            lead={'ID':'81','TITLE':'Synthetic lead','DATE_CREATE':'2026-09-01T00:00:00Z',
                  'DATE_MODIFY':'2026-11-13T09:00:00Z','STATUS_ID':'NEW',
                  'STATUS_SEMANTIC_ID':'P','SOURCE_ID':'WEB','UTM_SOURCE':'synthetic'}
            lead_rdf=transform_event(cfg,mappings,{'payload':lead},'lead/81/event.json')
            self.assertIn(b'https://example.org/bitrix24/lead/81',lead_rdf)

            deal=json.loads((ROOT/'tests/bitrix24/fixtures/snapshot-2026-11-04/deal.json').read_text())
            deal['LEAD_ID']='81'
            deal_rdf=transform_event(cfg,mappings,{'payload':deal},'deal/900001/event.json')
            self.assertIn(b'https://example.org/bitrix24/lead/81',deal_rdf)
            del deal['LEAD_ID']
            deal_without_lead=transform_event(cfg,mappings,{'payload':deal},'deal/900001/event.json')
            self.assertNotIn(b'https://example.org/bitrix24/lead/',deal_without_lead)

            activity={'ID':'7','SUBJECT':'Synthetic activity','OWNER_ID':'81','OWNER_TYPE_ID':'1',
                      'TYPE_ID':'2','PROVIDER_ID':'EMAIL','LAST_UPDATED':'2026-11-13T09:30:00Z'}
            activity_rdf=transform_event(cfg,mappings,{'payload':activity,'bindings':[
                {'entityTypeId':1,'entityId':81},{'entityTypeId':2,'entityId':81},
                {'entityTypeId':2,'entityId':82}]},
                'activity/7/event.json')
            self.assertIn(b'https://example.org/bitrix24/lead/81',activity_rdf)
            self.assertIn(b'https://example.org/bitrix24/deal/81',activity_rdf)
            self.assertIn(b'https://example.org/bitrix24/deal/82',activity_rdf)

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




if __name__ == '__main__':
    unittest.main()
