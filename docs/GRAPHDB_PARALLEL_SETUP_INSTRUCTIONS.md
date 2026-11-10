# Инструкция: параллельная связка GraphDB

Инструкция создаёт отдельную связку **S3 → GraphDB → монитор**, переносит в GraphDB актуальные именованные графы Fuseki и продолжает синхронизацию с копии checkpoint. Существующая связка Fuseki сохраняет собственные контейнеры, конфигурацию и Docker volume.

## 1. Что понадобится

- Запущенный Fuseki с текущей связкой `semantic_bi`.
- Запущенный лицензированный GraphDB с пустым repository `fuseki_migration_monitor`.
- Docker Compose, `curl`, Python 3 и доступный локально скрипт `export_json_to_nquads.py`.
- Конфигурация `deploy/loader/config.yaml` с S3 credentials и параметрами Fuseki. Файл остаётся локальным, не печатайте его и не добавляйте в Git.
- Образ загрузчика с поддержкой GraphDB. Для текущего окружения использовался `sha-1c7d3d3be69bcc3e08035f9ce3bc83423dfdcc12`.
- Мониторный образ с поддержкой GraphDB. В текущем окружении он локально собран как `ghcr.io/antonmish822/semantic-bi-monitor:local`; этот тег не опубликован в GHCR. Для другого сервера используйте опубликованный релизный образ, содержащий исправление GraphDB-запросов.

Все команды Compose ниже выполняются из корня `Semantic_BI`:

```bash
cd "/home/user/Downloads/_current_projects/Гарпикс/_разовое/bitrix24_dashboard/Semantic_BI"
export LOADER_UID="$(id -u)" LOADER_GID="$(id -g)"
export LOADER_IMAGE_TAG=sha-1c7d3d3be69bcc3e08035f9ce3bc83423dfdcc12
export MONITOR_IMAGE_TAG=local
export GRAPHDB_DOCKER_NETWORK=graphdb_default
export GRAPHDB_MONITOR_PORT=8081
```

Если имена Docker network отличаются, задайте фактические имена. Compose GraphDB подключается к внешней сети `graphdb_default`; Fuseki-контейнеры и существующий загрузчик используют `fuseki_default`.

## 2. Проверить целевой repository GraphDB

Убедитесь, что GraphDB доступен и `fuseki_migration_monitor` существует:

```bash
curl -fsS http://127.0.0.1:7200/rest/repositories/fuseki_migration_monitor
```

Проверьте, что в repository нет данных. Запрос должен вернуть нули:

```bash
curl -fsS -G \
  -H 'Accept: application/sparql-results+json' \
  --data-urlencode 'query=SELECT (COUNT(DISTINCT ?g) AS ?graphs) (COUNT(*) AS ?triples) WHERE { GRAPH ?g { ?s ?p ?o } }' \
  http://127.0.0.1:7200/repositories/fuseki_migration_monitor
```

Если repository не пуст, остановитесь: повторный N-Quads import добавит данные и не удалит графы, которых нет в свежем снимке. Используйте новый пустой repository или отдельно согласуйте очистку.

Для repository нужны ruleset `owl2-rl` и включённый `enableContextIndex`. Проверьте значение `enableContextIndex` через API:

```bash
curl -fsS http://127.0.0.1:7200/rest/repositories/fuseki_migration_monitor \
  | python3 -c 'import json,sys; d=json.load(sys.stdin); print(d["params"]["ruleset"]["value"], d["params"]["enableContextIndex"]["value"])'
```

Ожидается `owl2-rl true`. Если индекс отключён, включите его в GraphDB Workbench через **Setup → Repositories → Edit**, сохраните параметры и перезапустите только `fuseki_migration_monitor`. GraphDB применяет изменённую конфигурацию после перезапуска repository. [Документация GraphDB](https://graphdb.ontotext.com/documentation/11.2/configuring-a-repository.html)

## 3. Подготовить отдельные секретные конфиги

Создайте конфигурацию GraphDB-загрузчика из шаблона:

```bash
umask 077
cp deploy/graphdb/config.example.yaml deploy/graphdb/config.yaml
chmod 600 deploy/graphdb/config.yaml
```

В `deploy/graphdb/config.yaml` задайте те же bucket, portal ID и S3 credentials, что у текущей связки; проверьте параметры:

```yaml
checkpoint:
  path: "/var/lib/semantic-bi/state.sqlite3"
backend: graphdb
graphdb:
  url: "http://graphdb:7200"
  repository: "fuseki_migration_monitor"
```

Создайте конфигурацию монитора, сохранив S3-поля из текущего `rdf_store_monitor/.env`:

```bash
cp rdf_store_monitor/.env rdf_store_monitor/.env.graphdb
chmod 600 rdf_store_monitor/.env.graphdb
```

В `rdf_store_monitor/.env.graphdb` удалите или замените настройки `FUSEKI_*` и задайте:

```dotenv
RDF_BACKEND=graphdb
RDF_URL=http://graphdb:7200
GRAPHDB_REPOSITORY=fuseki_migration_monitor
RDF_USERNAME=
RDF_PASSWORD=
GRAPH_PREFIX=urn:bitrix24:entity:garpix-bitrix24-ru:
LOADER_STATUS_PATH=/var/lib/semantic-bi/status.json
```

Если GraphDB требует авторизацию, задайте `RDF_USERNAME` и `RDF_PASSWORD`. Файлы `deploy/graphdb/config.yaml` и `rdf_store_monitor/.env.graphdb` исключены из Git.

Проверьте Compose-конфигурацию и наличие сети GraphDB:

```bash
docker network inspect "$GRAPHDB_DOCKER_NETWORK" >/dev/null
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml config --quiet
```

## 4. Зафиксировать границу синхронизации Fuseki

Сначала дождитесь завершения цикла Fuseki-загрузчика. В status API состояние должно быть `waiting`, `error` должно быть `null`, а `last_run_id` — записан:

```bash
curl -fsS http://127.0.0.1:8080/api/status \
  | python3 -c 'import json,sys; d=json.load(sys.stdin); print(json.dumps(d["sync"],ensure_ascii=False))'
```

Сохраните `last_run_id` из ответа:

```bash
export MIGRATION_LAST_RUN_ID='<last_run_id из ответа>'
```

Остановите только Fuseki-загрузчик. Fuseki, монитор и GraphDB не останавливайте:

```bash
docker compose -p semantic_bi -f deploy/loader/compose.yaml stop loader
```

Пока загрузчик остановлен, сформируйте и экспорт, и резервную копию его checkpoint. Это одна логическая граница переноса.

## 5. Выгрузить актуальные именованные графы Fuseki

Экспортёр получает полный SPARQL Results JSON из текущего Fuseki по его внутренней Docker-сети, затем формирует N-Quads, CSV-манифест и отдельный файл на каждый граф. Для каждого запуска используйте новый каталог выгрузки:

```bash
export MIGRATION_DIR="/tmp/semantic-bi-graphdb-$(date -u +%Y%m%dT%H%M%SZ)"
export EXPORT_DIR="$MIGRATION_DIR/export"
mkdir -m 700 -p "$EXPORT_DIR"
export FUSEKI_EXPORTER="/home/user/Downloads/_current_projects/МИСИС/_учебное/практика_2026/cccr/graphdb/export_json_to_nquads.py"
```

Выполните SPARQL-экспорт. Запрос должен содержать `ORDER BY ?g`, так как конвертер проверяет порядок графов:

```bash
set -o pipefail
docker run --rm --network fuseki_default \
  --user "$LOADER_UID:$LOADER_GID" \
  --mount "type=bind,src=$PWD/deploy/loader/config.yaml,dst=/tmp/loader-config.yaml,readonly" \
  --entrypoint python \
  "ghcr.io/antonmish822/semantic-bi-loader:$LOADER_IMAGE_TAG" \
  -c 'import sys,yaml,requests; c=yaml.safe_load(open("/tmp/loader-config.yaml")); s=c[c["backend"]]; q="SELECT ?g ?s ?p ?o WHERE { GRAPH ?g { ?s ?p ?o } } ORDER BY ?g"; r=requests.get(s["url"].rstrip("/")+"/query",params={"query":q},headers={"Accept":"application/sparql-results+json"},auth=(s.get("username"),s.get("password")) if s.get("username") else None,timeout=600); r.raise_for_status(); sys.stdout.buffer.write(r.content)' \
  | python3 "$FUSEKI_EXPORTER" --output-dir "$EXPORT_DIR"
```

Скрипт должен вывести число графов и триплетов. В выполненном переносе было 13 929 графов и 212 176 триплетов. Это контрольные значения именно того снимка; при повторном переносе ожидаемые числа могут измениться.

## 6. Создать согласованную копию checkpoint

Создайте SQLite backup из остановленного volume. Источник монтируется отдельно; исходный checkpoint команда не изменяет:

```bash
docker run --rm \
  --user "$LOADER_UID:$LOADER_GID" \
  -v semantic_bi_loader_state_migrated:/source \
  -v "$MIGRATION_DIR:/out" \
  --entrypoint python \
  "ghcr.io/antonmish822/semantic-bi-loader:$LOADER_IMAGE_TAG" \
  -c 'import sqlite3; src=sqlite3.connect("/source/state.sqlite3"); dst=sqlite3.connect("/out/fuseki-checkpoint.sqlite3"); src.backup(dst); print("integrity",dst.execute("PRAGMA integrity_check").fetchone()[0]); print("last_run_id",dst.execute("SELECT value FROM meta WHERE key=\"last_run_id\"").fetchone()[0]); print("active_entities",dst.execute("SELECT COUNT(*) FROM entities WHERE deleted=0").fetchone()[0]); dst.close(); src.close()'
chmod 600 "$MIGRATION_DIR/fuseki-checkpoint.sqlite3"
```

Убедитесь, что `integrity` равно `ok`, `last_run_id` совпадает с `$MIGRATION_LAST_RUN_ID`, а активные `graph_uri` из checkpoint присутствуют в `manifest.csv`. Дополнительные графы, например графы онтологии, могут присутствовать в RDF-экспорте и не иметь записей в таблице `entities`.

## 7. Возобновить исходную связку и импортировать снимок

Как только экспорт и checkpoint backup готовы, возобновите Fuseki-загрузчик. Он продолжит с собственного checkpoint, пока выполняются импорт и подготовка GraphDB:

```bash
docker compose -p semantic_bi -f deploy/loader/compose.yaml start loader
```

Загрузите N-Quads в пустой `fuseki_migration_monitor`:

```bash
curl --fail --silent --show-error --max-time 1200 \
  -X POST -H 'Content-Type: application/n-quads' \
  --data-binary "@$EXPORT_DIR/named-graphs.nq" \
  -o "$MIGRATION_DIR/graphdb-import-response" \
  -w 'HTTP %{http_code}\n' \
  http://127.0.0.1:7200/repositories/fuseki_migration_monitor/statements
```

Ожидается HTTP `204`. N-Quads сохраняет IRI графов при импорте в GraphDB. [Документация GraphDB по импорту RDF](https://graphdb.ontotext.com/documentation/11.1/loading-data-using-the-workbench.html)

Сверьте GraphDB с `manifest.csv` по каждому графу. Следующий код запрашивает количество триплетов по всем графам и завершится с ошибкой при отличии:

```bash
python3 - "$EXPORT_DIR/manifest.csv" <<'PY'
import csv, json, sys, urllib.parse, urllib.request

manifest_path = sys.argv[1]
query_url = "http://127.0.0.1:7200/repositories/fuseki_migration_monitor"
query = "SELECT ?g (COUNT(*) AS ?triples) WHERE { GRAPH ?g { ?s ?p ?o } } GROUP BY ?g ORDER BY ?g"
body = urllib.parse.urlencode({"query": query}).encode()
request = urllib.request.Request(
    query_url, data=body,
    headers={"Accept": "application/sparql-results+json",
             "Content-Type": "application/x-www-form-urlencoded"},
    method="POST",
)
with urllib.request.urlopen(request, timeout=600) as response:
    result = json.load(response)
actual = {row["g"]["value"]: int(row["triples"]["value"])
          for row in result["results"]["bindings"]}
with open(manifest_path, encoding="utf-8", newline="") as source:
    expected = {row["graph_iri"]: int(row["triples"])
                for row in csv.DictReader(source)}
missing, extra = expected.keys() - actual.keys(), actual.keys() - expected.keys()
mismatched = [g for g in expected.keys() & actual.keys() if expected[g] != actual[g]]
print("export", len(expected), "graphs /", sum(expected.values()), "triples")
print("GraphDB", len(actual), "graphs /", sum(actual.values()), "triples")
print("missing", len(missing), "extra", len(extra), "per-graph count differences", len(mismatched))
if missing or extra or mismatched:
    print("sample differences:", list(sorted(missing))[:5],
          list(sorted(extra))[:5], list(sorted(mismatched))[:5])
    raise SystemExit(1)
print("MATCH: graph IRIs and per-graph triple counts are identical")
PY
```

**Не запускайте GraphDB-загрузчик до успешной проверки.**

## 8. Перепривязать копию checkpoint к GraphDB

Скрипт создаёт новый файл, проверяет исходную привязку, `last_run_id` и SQLite integrity, затем меняет backend и endpoint только в копии:

```bash
python3 scripts/rebind_checkpoint.py \
  --source "$MIGRATION_DIR/fuseki-checkpoint.sqlite3" \
  --destination "$MIGRATION_DIR/graphdb-checkpoint.sqlite3" \
  --expected-last-run-id "$MIGRATION_LAST_RUN_ID" \
  --target-identity 'http://graphdb:7200/repositories/fuseki_migration_monitor/rdf-graphs/service'
chmod 600 "$MIGRATION_DIR/graphdb-checkpoint.sqlite3"
```

Создайте отдельный volume и поместите туда копию под именем `state.sqlite3`. Каталог и файл должны принадлежать UID/GID загрузчика, чтобы он мог создать SQLite lock-файл:

```bash
docker volume create semantic_bi_graphdb_state
docker run --rm --user 0:0 \
  -v semantic_bi_graphdb_state:/target \
  -v "$MIGRATION_DIR:/migration:ro" \
  --entrypoint sh \
  "ghcr.io/antonmish822/semantic-bi-loader:$LOADER_IMAGE_TAG" \
  -c "cp /migration/graphdb-checkpoint.sqlite3 /target/state.sqlite3 && chown $LOADER_UID:$LOADER_GID /target /target/state.sqlite3 && chmod 600 /target/state.sqlite3"
```

Volume `semantic_bi_loader_state_migrated` Fuseki и checkpoint GraphDB должны оставаться разными.

## 9. Запустить GraphDB-загрузчик и монитор

Для текущей машины уже подготовлены локальные `deploy/graphdb/config.yaml` и `rdf_store_monitor/.env.graphdb`. Проверьте, что private-конфиги заполнены, затем запустите только новый Compose-проект:

```bash
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml config --quiet
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml up -d
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml ps
```

Следите за логом загрузчика:

```bash
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml logs -f loader
```

Он должен продолжить с `$MIGRATION_LAST_RUN_ID`, обработать появившиеся позже incremental runs и сохранить более новый `last_run_id`. Важно, чтобы нужные incremental manifests всё ещё были доступны в S3.

## 10. Проверить результат

Проверьте GraphDB-монитор:

```bash
curl -fsS http://127.0.0.1:8081/api/status \
  | python3 -c 'import json,sys; d=json.load(sys.stdin); print(json.dumps({"sync":d["sync"],"store":d["store"]},ensure_ascii=False))'
```

Ожидается `store.backend=graphdb`, `store.available=true`, `sync.state=waiting`, `sync.error=null`, `sync.caught_up=true`. Адрес UI: <http://127.0.0.1:8081>.

Сравните сводные числа по типам через `/api/stats` для `8081` и существующего монитора `8080`. После догонки S3 оба экземпляра должны показывать одинаковые данные; во время нового S3-цикла их курсоры могут временно отличаться.

## Управление и откат

Остановить только GraphDB-связку, не затрагивая Fuseki:

```bash
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml stop
```

Повторный запуск:

```bash
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml start
```

Не удаляйте `semantic_bi_graphdb_state`, если нужно сохранить позицию синхронизации. Для новой первичной миграции используйте новый пустой GraphDB repository и новый checkpoint-volume; не повторяйте импорт поверх уже заполненного repository.
