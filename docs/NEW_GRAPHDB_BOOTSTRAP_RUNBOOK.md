# Первая загрузка S3 → новый GraphDB repository

Инструкция для развёртывания нового пустого GraphDB repository и его первичного заполнения из S3 по контракту v2. Она предполагает, что экспортёр уже публикует в бакете контракт v2 и полный базовый snapshot. Для переключения экспортёра на v2 используйте отдельный [runbook cutover](../../bitrix24_s3_export/docs/CONTRACT_V2_CUTOVER_RUNBOOK.md).

## Что будет загружено

Первая загрузка читает из S3 `state.json`, указанный в нём базовый manifest и все JSON-события, на которые ссылается manifest. Затем она читает из S3 incremental manifests и их события до текущей подтверждённой вершины. В поле `mode` базового манифеста стоит `local_snapshot`: «local» относится к способу создания снимка экспортёром, а не к месту чтения загрузчиком. **Копировать snapshot на машину GraphDB или указывать путь к локальным файлам данных не нужно.** Локально создаётся только SQLite checkpoint загрузчика и его `status.json`.

Начальный snapshot содержит данные в пределах прав экспортёра: сделки и справочники выгружаются полностью, активности ограничены последними 90 днями на момент снимка. Отсутствие более старой активности в GraphDB ожидаемо. Лиды не входят в полный snapshot; отдельный backfill лидов, если он нужен и опубликован, применяется отдельно.

## 1. Подготовить машину

Нужны:

- работающий GraphDB с сетевой доступностью с машины загрузчика;
- доступ GraphDB для создания repository и учётная запись загрузчика с правом записи в него;
- доступ на чтение S3: `GetObject` для `state.json`, базового и incremental manifests и всех событий, на которые они ссылаются; `ListBucket` для нужного префикса может понадобиться при диагностике;
- Docker Engine с Compose, `yc` CLI с настроенным профилем для проверки S3 и доступ к двум опубликованным образам GHCR;
- checkout всего репозитория `Semantic_BI`. Локальная сборка Python и `cccr_engine` не нужна.

Убедитесь, что машина может разрешить hostname GraphDB и выйти к S3 endpoint по HTTPS. Не помещайте S3 и GraphDB пароли в командную строку, shell history или Git.

## 2. Создать пустой GraphDB repository

В GraphDB Workbench откройте **Setup → Repositories → Create new repository**, выберите тип **GraphDB repository**, задайте Repository ID, например `bitrix24-v2`, и сохраните. Репозиторий должен быть новым и пустым. Запишите:

- базовый URL GraphDB, например `http://graphdb.example.net:7200`;
- Repository ID, например `bitrix24-v2`;
- username/password пользователя с записью.

В конфигурации загрузчика указывается базовый URL сервера, без `/repositories/...`. Проверьте доступность repository через Workbench. Не используйте существующий repository с данными: загрузчик может обновлять и удалять именованные графы с namespace из конфигурации.

## 3. Подготовить checkout и образы

Перейдите в каталог `Semantic_BI`:

```bash
cd /path/to/checkout/Semantic_BI
```

Оба образа находятся в приватном GHCR: `ghcr.io/antonmish822/semantic-bi-loader` и `ghcr.io/antonmish822/semantic-bi-monitor`. Войдите с GitHub token, имеющим `read:packages`, и укажите существующий release или SHA tag:

```bash
docker login ghcr.io
export IMAGE_TAG=v2.0.0
export LOADER_UID=$(id -u)
export LOADER_GID=$(id -g)
```

Укажите существующий опубликованный тег вашей версии; оба контейнера должны использовать один тег. `v2.0.0` — тег, записанный в подготовленном пакете удалённого запуска. Доступные типы тегов описаны в [контракте контейнерных образов](CONTAINER_IMAGE_CONTRACT.md). Команда `docker login` запросит credentials интерактивно. Образы опубликованы для `linux/amd64`.

## 4. Создать конфигурацию загрузчика

Создайте отдельный файл, не добавляйте его в Git:

```bash
cp deploy/loader/config.example.yaml deploy/loader/config.yaml
chmod 600 deploy/loader/config.yaml
```

Заполните его. Пути ниже относятся к файловой системе контейнера:

```yaml
s3:
  bucket: "ВАШ_BUCKET"
  portal_id: "ВАШ_PORTAL_ID"
  endpoint_url: "https://storage.yandexcloud.net"
  region: "ru-central1"
  access_key: "ВАШ_S3_ACCESS_KEY_ID"
  secret_key: "ВАШ_S3_SECRET_ACCESS_KEY"

mapping:
  registry_path: "/app/mappings/bitrix24/registry.json"
  schemas_root: "/app/mappings/bitrix24"

checkpoint:
  path: "/var/lib/semantic-bi/state.sqlite3"
  resume_mode: strict

backend: graphdb
graphdb:
  url: "http://host.docker.internal:7200"
  repository: "bitrix24-v2"
  username: "LOADER_USER"
  password: "GRAPHDB_PASSWORD"
  timeout_seconds: 90
  graph_namespace: "urn:bitrix24:entity"

snapshot_reconcile_entity_types: [deal, user, category, stage, source]
poll_interval_seconds: 60
```

`portal_id` должен совпадать с префиксом данных экспортёра в S3. Для него код строит путь `bitrix24/<portal_id>/`. Перепроверьте его по расположению `state.json` в бакете.

Оставьте `resume_mode: strict` для первичной загрузки и приёмки. Не добавляйте `lead` в `snapshot_reconcile_entity_types`: текущий код запрещает сверку лидов как полного snapshot. Перечень reconciliation оставьте по умолчанию, только если экспортёр подтверждает полноту этих типов; он влияет на удаление из GraphDB сущностей, отсутствующих в полном snapshot. Активности не включайте. Для исходного рабочего v2 бакета используйте `bucket: garpix-bitrix24-v2-parallel-20261114` и `portal_id: garpix-bitrix24-ru-v2-parallel`; пакет передачи содержит готовые значения и ключи.

Compose создаст новый отдельный том `loader_state`; путь checkpoint внутри контейнера должен остаться `/var/lib/semantic-bi/state.sqlite3`. Для данного repository используйте собственный checkpoint и далее всегда сохраняйте пару GraphDB repository + этот checkpoint вместе. Не используйте для чистого запуска обычный `deploy/loader/compose.yaml`: он подключает внешний том прежней установки.

Пример `host.docker.internal:7200` подходит, когда GraphDB доступен на хосте с Docker через порт 7200. GraphDB должен слушать адрес, достижимый из контейнеров; одной привязки к `127.0.0.1` хоста недостаточно. Если GraphDB находится на другой машине или в общей Docker сети, укажите доступное из контейнера имя/URL и при необходимости присоедините оба сервиса к этой сети.

Создайте `rdf_store_monitor/.env` с правами `0600` по образцу:

```bash
cp rdf_store_monitor/.env.example rdf_store_monitor/.env
chmod 600 rdf_store_monitor/.env
```

Задайте в нём как минимум:

```dotenv
RDF_BACKEND=graphdb
RDF_URL=http://host.docker.internal:7200
GRAPHDB_REPOSITORY=bitrix24-v2
RDF_USERNAME=LOADER_USER
RDF_PASSWORD=GRAPHDB_PASSWORD
GRAPH_PREFIX=urn:bitrix24:entity:ВАШ_PORTAL_ID:
LOADER_STATUS_PATH=/var/lib/semantic-bi/status.json
S3_BUCKET=ВАШ_BUCKET
S3_PORTAL_ID=ВАШ_PORTAL_ID
S3_ENDPOINT_URL=https://storage.yandexcloud.net
S3_REGION=ru-central1
AWS_ACCESS_KEY_ID=ВАШ_S3_ACCESS_KEY_ID
AWS_SECRET_ACCESS_KEY=ВАШ_S3_SECRET_ACCESS_KEY
```

Монитор читает GraphDB, S3 и `status.json` из общего тома. Его порт опубликован только на `127.0.0.1:8080` хоста. Для удалённого просмотра используйте SSH tunnel.

Если передаёте подготовленный пакет `local-data/remote-graphdb-v2-20261115`, его `README.md` содержит готовые файлы и команды именно для этого аккаунта и бакета; заполните в нём только параметры GraphDB и примените выбранный опубликованный tag. Секреты из этого каталога не добавляйте в Git.

## 5. Проверить конфигурацию и исходный контракт

Сверьте параметры S3 и GraphDB, затем убедитесь, что v2 state доступен по префиксу `bitrix24/<portal_id>/state.json`, имеет `contract_version: 2`, а его `base_manifest_key` указывает на snapshot manifest той же `epoch_id`. Manifest должен иметь `mode: local_snapshot`, `status: complete`, `complete: true` и SHA, указанный в state. Он и все его `objects[].key` должны уже находиться в S3. Проверки содержимого событий и их SHA загрузчик выполнит сам в strict режиме.

Если на машине есть `yc` CLI, проверьте объекты с профилем, имеющим право читать этот бакет. В пакете передачи лежит ключ сервисного аккаунта для `yc`; S3 ключ в `config.yaml`/`.env` используется контейнерами. Настройте профиль по README пакета. `head-object` подтверждает наличие объекта, `get-object` скачивает JSON для просмотра. Сначала проверьте и прочитайте state:

```bash
yc storage s3api head-object \
  --bucket ВАШ_BUCKET \
  --key bitrix24/ВАШ_PORTAL_ID/state.json
yc storage s3api get-object \
  --bucket ВАШ_BUCKET \
  --key bitrix24/ВАШ_PORTAL_ID/state.json \
  /tmp/bitrix24-v2-state.json
cat /tmp/bitrix24-v2-state.json
```

Возьмите точный `base_manifest_key` из state и проверьте базовый манифест:

```bash
yc storage s3api head-object \
  --bucket ВАШ_BUCKET \
  --key КЛЮЧ_ИЗ_base_manifest_key
yc storage s3api get-object \
  --bucket ВАШ_BUCKET \
  --key КЛЮЧ_ИЗ_base_manifest_key \
  /tmp/bitrix24-v2-base-manifest.json
cat /tmp/bitrix24-v2-base-manifest.json
sha256sum /tmp/bitrix24-v2-base-manifest.json
```

Сравните SHA-256 с `base_manifest_sha256` из state и убедитесь, что `objects` содержит ссылки на события в S3. Это временные файлы только для предварительной проверки; загрузчик их не использует. Не публикуйте вывод с внутренними данными. Перед загрузкой проверьте, что `backend: graphdb`, Repository ID и URL ведут именно в новый пустой repository, а том checkpoint будет новым. Синтаксис команд `yc` приведён в [документации Yandex Cloud](https://yandex.cloud/ru/docs/storage/operations/objects/download).

## 6. Скачать образы и выполнить первичный цикл

Сначала скачайте оба образа. Затем запустите только загрузчик на один цикл:

```bash
docker compose -p semantic_bi_graphdb_v2 -f deploy/loader/compose.new-graphdb.yaml pull
docker compose -p semantic_bi_graphdb_v2 -f deploy/loader/compose.new-graphdb.yaml run --rm loader \
  --config /run/secrets/loader_config --once
```

Если используете передаваемый пакет, перейдите в его каталог и применяйте те же команды с `-f compose.yaml`; в package README приведён готовый вариант.

Первый цикл может быть долгим: он скачивает события полного базового snapshot непосредственно из S3, проверяет каждый объект и записывает сущности в GraphDB, затем догоняет incremental цепочку из S3. В томе хранятся только checkpoint и статус; файлы snapshot туда класть не требуется. Не запускайте второй экземпляр одновременно с тем же checkpoint. Не удаляйте том и не создавайте новый repository, если процесс прервался: исправьте причину и повторите ту же команду — загрузчик продолжит безопасно.

В передаваемой конфигурации `contract_v2_limits.max_cycle_read_seconds` равен 28 800 секундам (8 часов), а `max_objects_per_cycle` по умолчанию — 1 000 000. Если объём исходного snapshot требует больше времени или объектов, увеличьте соответствующий лимит в `config.yaml` и повторите запуск с тем же томом. Превышение лимита останавливает цикл с ошибкой; уже записанный checkpoint сохраняется.

Статус записывается рядом с SQLite в общем томе. После выхода с кодом 0 запустите монитор и откройте `http://127.0.0.1:8080` на сервере:

```bash
docker compose -p semantic_bi_graphdb_v2 -f deploy/loader/compose.new-graphdb.yaml up -d monitor
docker compose -p semantic_bi_graphdb_v2 -f deploy/loader/compose.new-graphdb.yaml logs --tail=100 monitor
```

Ожидайте `contract_version: 2`, `state: idle`, заполненные `epoch_id` и `base_manifest_sha256`. Если в S3 state есть incremental tip, `applied_tip_run_id` и `applied_tip_manifest_sha256` должны совпасть с ним при неизменном state. Если tip ещё нет, эти поля могут быть пустыми. Если экспортёр работал параллельно, загрузчик применяет только вершину из прочитанного state; следующий цикл обработает более новую подтверждённую вершину.

При ошибке команда завершится ненулевым кодом, а подробности останутся в логе. `status.json` покажет `blocked` или `error`, но не содержит payload и секреты. Исправьте доступ/источник и повторите с тем же checkpoint. Не меняйте SHA в SQLite вручную.

## 7. Проверить результат в GraphDB

В Workbench выберите новый repository и выполните SPARQL-запросы.

Количество именованных графов:

```sparql
SELECT (COUNT(DISTINCT ?g) AS ?graphs)
WHERE { GRAPH ?g { ?s ?p ?o } }
```

Количество троек:

```sparql
SELECT (COUNT(*) AS ?triples)
WHERE { GRAPH ?g { ?s ?p ?o } }
```

Проверка namespace целевой проекции:

```sparql
SELECT ?g (COUNT(*) AS ?triples)
WHERE {
  GRAPH ?g { ?s ?p ?o }
  FILTER(STRSTARTS(STR(?g), "urn:bitrix24:entity"))
}
GROUP BY ?g
ORDER BY DESC(?triples)
LIMIT 20
```

Числа графов/троек зависят от данных портала и не являются заранее фиксированными. Сопоставьте их с количеством объектов и counts базового манифеста и incremental runs, учитывая, что повторные версии одной сущности заменяют её граф, а delete удаляет граф. Проверьте несколько известных сущностей и связи в Workbench, а также отсутствие `blocked`/`error` в `status.json`. Первичный цикл считается принятым, когда snapshot и chain tip подтверждены статусом, контрольные данные совпадают с порталом, и объём данных выглядит ожидаемо.

Для строгого полного исторического аудита повторно загрузите в отдельный пустой тестовый repository с новым checkpoint в режиме `strict`; не сбрасывайте рабочую базу ради аудита.

## 8. Включить регулярное обновление

После успешной проверки включите постоянную работу загрузчика:

```bash
docker compose -p semantic_bi_graphdb_v2 -f deploy/loader/compose.new-graphdb.yaml up -d loader
docker compose -p semantic_bi_graphdb_v2 -f deploy/loader/compose.new-graphdb.yaml logs -f loader
```

Процесс выполняет цикл каждые `poll_interval_seconds` (в примере 60 секунд). Compose перезапускает контейнер после сбоя. При рестарте сохраняйте ту же конфигурацию, тот же том checkpoint и тот же GraphDB repository. Не запускайте одновременно два writer-процесса с одним checkpoint.

При остановке процесса SQLite checkpoint нужно сохранить. Для backup используйте описанный в [архитектуре backup/restore](BACKUP_RESTORE_ARCHITECTURE.md) backup barrier и сохраняйте GraphDB + SQLite как согласованную пару. Простое копирование SQLite во время активной записи не является проверенной резервной копией пары.

## Если нужно начать первичную загрузку заново

Остановите загрузчик. Создайте новый пустой GraphDB repository и новый путь checkpoint, затем повторите шаги с новым именем repository и checkpoint. Не очищайте существующую базу или удаляйте SQLite, пока не проверено, что их больше не использует работающий экземпляр и что нужные данные/backup сохранены. Начинать только с пустого SQLite при уже заполненном repository можно лишь как осознанную повторную проекцию; для чистой первичной загрузки используйте оба новых ресурса.

## Диагностика

- **`state.json`/manifest отсутствует или contract version не 2:** это не v2 источник; проверьте bucket, `portal_id`, endpoint и состояние cutover экспортёра.
- **Ошибка GraphDB или 401/403:** проверьте URL сервера, Repository ID, сетевой доступ и права пользователя. URL в конфигурации должен быть базовым URL сервера.
- **Недоступен объект или не совпадает SHA:** остановитесь на ошибке, проверьте доступность и целостность объекта в S3; не обходите строгую проверку.
- **`Checkpoint v2 не является предком` / конфликт store identity:** проверьте, что используется именно новый checkpoint, предназначенный для этого repository и этой S3 epoch.
- **Цикл продолжается, но не достигает ожидаемого tip:** проверьте цепочку run manifests, SHA/parent pointers и status-маркеры в S3; не пропускайте звено вручную.

Подробное описание гарантий контракта, checkpoint, статусов и восстановления находится в [LOADER_CONTRACT_V2_RUNBOOK.md](LOADER_CONTRACT_V2_RUNBOOK.md).
