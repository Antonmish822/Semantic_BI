# Параллельная связка GraphDB

Документ описывает отдельный S3→GraphDB загрузчик и монитор, работающие рядом с существующей связкой Fuseki. Связки используют одинаковый портал и S3-префикс, но разные repository, checkpoint-volume и статусные файлы.

## Состав

- GraphDB repository: `fuseki_migration_monitor`.
- Загрузчик: Compose-проект `semantic_bi_graphdb`, собственный volume `semantic_bi_graphdb_state`.
- Монитор: тот же volume только для чтения, адрес `http://127.0.0.1:8081`.
- S3: тот же bucket и portal prefix, доступ только для чтения.
- Fuseki продолжает работать со своим отдельным volume и конфигурацией.

Не подключайте GraphDB-загрузчик к `semantic_bi_loader_state_migrated` и не меняйте конфигурацию Fuseki-связки на GraphDB.

## Подготовка конфигурации

Скопируйте шаблон `deploy/graphdb/config.example.yaml` в `deploy/graphdb/config.yaml`, задайте S3 credentials и проверьте endpoint/repository. Создайте `rdf_store_monitor/.env.graphdb` с параметрами S3 мониторинга и настройками RDF:

```dotenv
RDF_BACKEND=graphdb
RDF_URL=http://graphdb:7200
GRAPHDB_REPOSITORY=fuseki_migration_monitor
RDF_USERNAME=
RDF_PASSWORD=
GRAPH_PREFIX=urn:bitrix24:entity:garpix-bitrix24-ru:
LOADER_STATUS_PATH=/var/lib/semantic-bi/status.json
```

Оба файла с credentials локальные и исключены из Git. Настройте права доступа `600`.

## Экспорт из Fuseki и импорт в GraphDB

Экспорт должен быть сделан при остановленном Fuseki-загрузчике, чтобы RDF-снимок и SQLite checkpoint имели одну границу обработки. Используйте SPARQL `SELECT ?g ?s ?p ?o WHERE { GRAPH ?g { ?s ?p ?o } } ORDER BY ?g` и конвертер `export_json_to_nquads.py`. Сохраняйте каждый снимок в новый output directory: скрипт не заменяет каталог с посторонними файлами.

Загрузите `named-graphs.nq` в `http://localhost:7200/repositories/fuseki_migration_monitor/statements` с `Content-Type: application/n-quads`. N-Quads сохраняет IRI именованных графов. До загрузки проверьте, что repository пустой, writable и у него включён context index.

Сверьте `manifest.csv` с GraphDB: множество графов и количество триплетов в каждом графе должны совпасть. Не запускайте GraphDB-загрузчик, пока проверка не прошла.

## Перепривязка checkpoint

При остановленном Fuseki-загрузчике создайте консистентный SQLite backup. Убедитесь, что `last_run_id` равен последнему запуску, включённому в RDF-экспорт. Затем создайте копию checkpoint для GraphDB:

```bash
python3 scripts/rebind_checkpoint.py \
  --source /path/to/fuseki-checkpoint.sqlite3 \
  --destination /path/to/graphdb-state.sqlite3 \
  --expected-last-run-id '<last_run_id из status.json>' \
  --target-identity 'http://graphdb:7200/repositories/fuseki_migration_monitor/rdf-graphs/service'
```

Команда проверяет SQLite integrity и курсор, оставляет исходную базу нетронутой и меняет привязку только в копии. Поместите созданный файл как `state.sqlite3` в отдельный volume `semantic_bi_graphdb_state`.

## Запуск и проверка

Из `Semantic_BI` задайте используемые теги образов, UID/GID и имя сети GraphDB. Для текущего локального GraphDB сеть обычно называется `graphdb_default`:

```bash
export LOADER_UID="$(id -u)" LOADER_GID="$(id -g)"
export LOADER_IMAGE_TAG=sha-<проверенный-sha>
export MONITOR_IMAGE_TAG=sha-<проверенный-sha>
export GRAPHDB_DOCKER_NETWORK=graphdb_default
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml up -d
```

Проверяйте логи независимого загрузчика:

```bash
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml logs -f loader
```

После запуска загрузчик должен сохранить исходный `last_run_id` либо перейти к более поздним incremental runs. Монитор GraphDB доступен по адресу `http://127.0.0.1:8081`; в `/api/status` ожидаются `store.backend=graphdb` и `store.available=true`.

Остановить только новую связку:

```bash
docker compose -p semantic_bi_graphdb -f deploy/graphdb/compose.yaml stop
```

Не удаляйте `semantic_bi_graphdb_state`: в нём находится самостоятельная позиция GraphDB-загрузчика.

## Результат переноса 2026-11-10

Для текущего окружения выполнен согласованный export/import:

- экспорт Fuseki: 13 929 графов и 212 176 триплетов;
- активные entity-графы checkpoint: 13 927; два дополнительных графа — `urn:bitrix24:ontology:activity-classification` и `urn:crm:ontology`;
- GraphDB `fuseki_migration_monitor` совпал с экспортом по множеству IRI и числу триплетов в каждом графе;
- у repository включён `enableContextIndex`;
- копия checkpoint была привязана к GraphDB с курсором `20261110T125036Z-6f81fe08`, затем загрузчик применил incremental runs и дошёл до `20261110T131018Z-f5863250`;
- монитор GraphDB на `http://127.0.0.1:8081` и монитор Fuseki показывают одинаковые числа графов по типам; оба загрузчика сообщали `caught_up=true` на момент проверки.

Локальный GraphDB-монитор использует образ `ghcr.io/antonmish822/semantic-bi-monitor:local`, собранный из текущего checkout с правкой SPARQL `GRAPH ?g { ?s ?p ?o }`. Этот локальный образ не опубликован в GHCR; для переноса на другой сервер сначала нужно включить изменения монитора в коммит и выпустить подходящий релизный образ.
