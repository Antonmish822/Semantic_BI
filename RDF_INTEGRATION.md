# Контракт Bitrix24 S3 → RDF

Этот документ описывает интерфейс между экспортёром Bitrix24 и отдельным S3→Fuseki-синхронизатором. Экспортёр пишет исходные JSON-файлы в Yandex Object Storage; RDF-проект только читает их и применяет к RDF-хранилищу.

## Подключение

```text
Endpoint: https://storage.yandexcloud.net
Bucket:   bitrix24-s3-export-b1g4igegv5dfr9pl3nag
Prefix:   bitrix24/garpix-bitrix24-ru/
Region:   ru-central1
```

Используйте выделенные учётные данные RDF-reader из локального файла `.env.rdf-reader` в корне подпроекта. Он исключён из Git. Секретный ключ нельзя коммитить, включать в логи, отправлять в тикеты или передавать открытым сообщением; передавайте файл/значения разработчикам по одобренному защищённому каналу. Учётная запись имеет S3 ACL `READ` на бакет: это чтение объектов и листинг бакета, без права записи, удаления или изменения ACL. ACL действует на весь бакет, поэтому в коде RDF-потребителя всегда ограничивайте листинг префиксом `bitrix24/garpix-bitrix24-ru/`.

Выделенная учётная запись создана для этого интеграционного потребителя:

```text
Service account:     bitrix24-rdf-reader
Service account ID:  ajerjo80nkatcv5siebd
Access key ID:       aje9hekbqcadq9kankj1  (resource ID for revocation)
```

Access key resource ID нужен для отзыва (`yc iam access-key delete aje9hekbqcadq9kankj1`). Не путайте его с `AWS_ACCESS_KEY_ID` из приватного файла: это разные идентификаторы. S3-доступ проверен: `ListObjectsV2` и `GetObject` под новым ключом успешно прочитали объект схемы 3; секрет не выводился.

Ожидаемые переменные окружения в файле:

```text
AWS_ACCESS_KEY_ID
AWS_SECRET_ACCESS_KEY
AWS_DEFAULT_REGION
S3_ENDPOINT_URL
S3_BUCKET
S3_PREFIX
```

Для boto3 передавайте `endpoint_url=S3_ENDPOINT_URL`, `region_name=AWS_DEFAULT_REGION`, `aws_access_key_id=AWS_ACCESS_KEY_ID` и `aws_secret_access_key=AWS_SECRET_ACCESS_KEY`. При проверке соединения используйте `ListObjectsV2` с `Prefix=S3_PREFIX` и `MaxKeys=1`, затем `GetObject`; не выводите тело объекта целиком в лог.

## Ключи объектов

```text
bitrix24/<portal_id>/state.json
bitrix24/<portal_id>/events/<entity>/<entity_identity>/<event_id>.json
bitrix24/<portal_id>/runs/<run_id>/manifest.json
bitrix24/<portal_id>/runs/<run_id>/status.json
```

Для этого портала `<portal_id>` равен `garpix-bitrix24-ru`. Экспортируемые сущности: `deal`, `activity`, `user`, `category`, `stage`, `source`. Для стадий и источников `entity_identity` составной: `<ENTITY_ID>:<STATUS_ID>`, например `DEAL_STAGE_2:NEW`. Двоеточие — часть S3-ключа.

`state.json` — внутреннее состояние экспортёра и курсоры Bitrix24. RDF-проект читает его для определения принятого полного снимка и верхней границы регулярных запусков. Это не журнал событий и не checkpoint RDF-синхронизатора; файл не изменяется потребителем.

## Формат события

Каждый объект в `events/` — UTF-8 JSON, канонически сериализованный (ключи отсортированы, без пробелов между разделителями). Текущая схема — `schema_version: 4`; загрузчик принимает версии 3 и 4, поскольку v4 добавляет поля к payload, не меняя envelope и идентичность сущности.

```json
{
  "schema_version": 4,
  "event_id": "<sha256 hex>",
  "graph_id": "urn:bitrix24:graph:garpix-bitrix24-ru:<event_id>",
  "entity_key": "garpix-bitrix24-ru/deal/23298",
  "operation": "upsert",
  "source_modified_at": "2026-11-04T11:07:14Z",
  "observed_at": "2026-11-04T11:07:14Z",
  "payload": {},
  "bindings": []
}
```

- `event_id` — SHA-256 строки `<portal>|<type>|<identity>|<source timestamp>|<digest>`, где `digest` — SHA-256 канонического JSON `{payload, bindings}`. Поэтому новая версия той же CRM-записи обычно получает новые `event_id` и `graph_id`.
- `graph_id` — версионное имя события в экспортном контракте; S3→Fuseki-синхронизатор не использует его как имя целевого графа.
- `entity_key` — стабильная идентичность Bitrix24: `<portal_id>/<entity>/<entity_identity>`. URI целевого графа строится как `urn:bitrix24:entity:<portal>:<type>:<sha256(entity_key)>`; все версии этой сущности заменяют один именованный граф.
- `operation` сейчас всегда `upsert`. Tombstone-событий и уведомлений об удалении нет.
- `source_modified_at` — исходное время изменения в UTC или `null`, если Bitrix24 его не вернул. `observed_at` также UTC; при отсутствии времени источника экспортёр ставит `1970-01-01T00:00:00Z`.
- `payload` — allowlist CRM-полей, не полный ответ REST API. Не считать отсутствующее поле командой очистить прежнее значение без применения правила замены всей версии.
- `bindings` — связи активности с CRM-сущностями: массив `{ "entityTypeId": <value>, "entityId": <value> }`; для прочих сущностей обычно пустой массив.

Сделки содержат, среди прочего, `ID`, `TITLE`, `DATE_CREATE`, `DATE_MODIFY`, `STAGE_ID`, `CATEGORY_ID`, `SOURCE_ID`, `ASSIGNED_BY_ID`, `COMPANY_ID`, `CONTACT_ID`, сумму и валюту. В schema v4 добавлены `STAGE_SEMANTIC_ID`, `LAST_ACTIVITY_TIME` и `LAST_ACTIVITY_BY`. Активности содержат `ID`, `TYPE_ID`, `PROVIDER_ID`, `PROVIDER_TYPE_ID`, `SUBJECT`, `DIRECTION`, статусы/даты, `RESPONSIBLE_ID`, `OWNER_TYPE_ID`, `OWNER_ID`, `PRIORITY` и связи. Пользовательские справочники содержат `ID`, `NAME`, `LAST_NAME`, `ACTIVE`; стадии/источники используют составной identity.

`event_id` является версионным и не даёт права считать события удалениями. При обновлении RDF-проекции потребитель заменяет содержимое стабильного именованного графа по `entity_key`. Текущий синхронизатор хранит `entity_key → event_id/graph_uri` и обработанные `run_id` в локальном SQLite checkpoint.

## Манифесты и порядок чтения

Обрабатывайте только объекты, на которые ссылается `manifest.json`. Не принимайте произвольный объект, найденный листингом `events/`: событие может быть загружено до публикации манифеста, а повторная доставка безопасна только после проверки манифеста и `event_id`.

### Полный локальный снимок

`runs/<run_id>/manifest.json` имеет `mode: "local_snapshot"`, `status: "complete"`, `complete: true`, `t0`, `catchup_to`, `activity_from`, `counts` и полный список `objects`. Каждый элемент списка:

```json
{
  "entity": "deal",
  "event_id": "<sha256 hex>",
  "graph_id": "urn:bitrix24:graph:garpix-bitrix24-ru:<event_id>",
  "key": "bitrix24/garpix-bitrix24-ru/events/deal/23298/<event_id>.json",
  "sha256": "<sha256 hex объекта>"
}
```

Это начальная база: синхронизатор обходит все ссылки из манифеста, валидирует ключ/JSON/хеш и строит RDF-проекцию. Сделки и справочники выгружаются полностью в рамках прав вебхука; активности ограничены 90 днями по `LAST_UPDATED` на момент снимка. Отсутствие активности в снимке не доказывает её удаление: она может быть старше окна. Точная последовательность начальной загрузки, сверки, инкрементального догоняния и хранения checkpoint описана в [логике загрузки S3 → Fuseki](docs/S3_TO_FUSEKI_LOADING.md).

### Инкрементальные запуски

Функция публикует `mode: "incremental"` раз в 10 минут. Такие манифесты содержат `window.from`, `window.to`, `created_at`, `counts`, `objects` и `dictionary_snapshot_id`. В них входят изменившиеся сделки/активности и периодические снимки справочников; это не полный список CRM.

Порядок обработки синхронизатором:

1. Читайте `state.json`; `adopted_local_manifest_key` и `adopted_local_manifest_sha256` указывают принятый базовый снимок, `last_run_id` — верхняя граница incremental-манифестов, видимая экспортёру. Сверяйте их с собственным постоянным SQLite checkpoint. Само значение `last_run_id` ещё не означает, что статус-маркер запуска успешно записан.
2. Сначала примените базовый `local_snapshot` manifest. Затем выберите необработанные манифесты `mode: incremental`, отсортируйте их по `window.to`/`created_at` и не считайте `backfill` обычным инкрементом.
3. Текущий загрузчик требует для incremental `manifest.status == "complete"` и соответствующий `status.json` с тем же `run_id` и `status == "complete"`. При отсутствующем или неуспешном статусе он останавливается на этом запуске и откладывает последующие; он не пропускает failed run. Поскольку exporter публикует manifest до завершения фиксации state/status, такой run может остаться видимым. Возможная блокировка очереди зарегистрирована как задача следующего этапа в [SYNC_RELIABILITY_BACKLOG.md](docs/SYNC_RELIABILITY_BACKLOG.md). У локального полного снимка `status.json` не создаётся; для него достаточно опубликованного манифеста с `complete: true` и `status: "complete"`.
4. Для каждого `objects[]` выполните `GetObject` по `key`, проверьте, что SHA-256 скачанных байтов совпадает с `sha256`, затем разберите JSON и проверьте соответствие `event_id`, `graph_id`, `entity_key` и ключа.
5. Применяйте версии идемпотентно: сохраняйте последний `event_id` для каждого `entity_key`. Окно экспорта перекрывается для надёжности, поэтому один объект/версия может встретиться повторно.
6. После каждого успешного PUT/DELETE сохраняйте состояние сущности в SQLite. Отмечайте `run_id` как обработанный только после успешного применения всех ссылок `objects[]`. При сбое повторно обработайте тот же манифест; повторы безопасны благодаря стабильному URI графа, Graph Store PUT/DELETE и проверке event/mapping version.

Exporter публикует incremental manifest до фиксации курсоров во внутреннем state store и обновления `state.json`; затем в `finally` пишет `status.json`. Синхронизатор использует только `status.json: complete` как признак завершённого вызова. `state.json` хранит состояние экспортёра, а не RDF-checkpoint. Синхронизатор хранит собственный checkpoint в SQLite и должен сохранять этот файл между перезапусками.

### Замена снимка

Скрипт `refresh_s3_snapshot.py` публикует/принимает новый полный снимок, удаляет из `events/` объекты, отсутствующие в его манифесте, и выполняет инкрементальный догон. `runs/` сохраняет исторические манифесты, поэтому старый manifest может ссылаться на уже удалённые event-файлы.

Если `state.json.adopted_local_manifest_key` изменился, S3→Fuseki-синхронизатор применяет новый полный снимок и после обработки сверяет сущности типов, включённых в `snapshot_reconcile_entity_types`. По умолчанию сверяются сделки и справочники (`deal`, `user`, `category`, `stage`, `source`); активность исключена, так как снимок покрывает только недавнее окно. Перечень сверяемых типов — свойство конфигурации синхронизатора. Важное ограничение: exporter при публикации нового снимка удаляет из `events/` старые файлы, отсутствующие в снимке, при этом исторические `runs/` манифесты остаются. Поэтому checkpoint синхронизатора должен быть актуальным; если он указывает на истёкший запуск или отстаёт почти на срок хранения S3, безопасное продолжение останавливается и требуется новый полный снимок. Удаления в Bitrix24 не приходят как tombstone; внеполосное удаление CRM-записи обнаруживается только для настроенных типов при успешной сверке с авторитетным снимком.

## Состав данных и эксплуатационные правила

Полное описание обработки — от локального получения snapshot до регулярного опроса инкрементов, повторов после сбоя и пере-проекции при изменении схем — находится в [docs/S3_TO_FUSEKI_LOADING.md](docs/S3_TO_FUSEKI_LOADING.md).

Ограничения текущего run ID и восстановления очереди после неуспешного запуска вынесены в [задачи следующего этапа](docs/SYNC_RELIABILITY_BACKLOG.md).

- Первый снимок включает все доступные сделки и справочники, плюс активности за последние 90 дней. Это не снимок всех исторических активностей.
- Ежедневное обновление справочников может повторно опубликовать объект с тем же `entity_key` и новым `event_id`.
- Backfill создаёт отдельный манифест и не сдвигает регулярные курсоры; по умолчанию не применять его в RDF без явного задания.
- Обычные incremental ID имеют UTC timestamp в начале. Сценарий замены snapshot-а вызывает catch-up с пользовательским ID `refresh-catchup-<timestamp>`, хотя загрузчик извлекает время из начала ID и использует лексикографические границы ID. Следующий обычный run может оказаться лексикографически меньше checkpoint. До исправления это известное ограничение; подробность и критерии исправления — в backlog.
- S3 lifecycle хранит `events/` и `runs/` до 90 дней. RDF-потребитель должен читать новые манифесты регулярно и не рассчитывать на бесконечную историю.
- Разовая очистка событий в S3 не удаляет данные из RDF. RDF-проект отвечает за собственную согласованность и удаление графов.
- Ниже контракт версионируется вместе с `schema_version`; несовместимые изменения схемы должны сопровождаться обновлением этого документа и consumer-а.

## Проверка доступа и пример

Загрузите переменные без вывода файла:

```bash
set -a
. ./.env.rdf-reader
set +a
```

Проверка списка (выводятся только число ключей и HTTP-результат):

```bash
python - <<'PY'
import os
import boto3

s3 = boto3.client(
    "s3",
    endpoint_url=os.environ["S3_ENDPOINT_URL"],
    region_name=os.environ["AWS_DEFAULT_REGION"],
    aws_access_key_id=os.environ["AWS_ACCESS_KEY_ID"],
    aws_secret_access_key=os.environ["AWS_SECRET_ACCESS_KEY"],
)
page = s3.list_objects_v2(
    Bucket=os.environ["S3_BUCKET"], Prefix=os.environ["S3_PREFIX"], MaxKeys=1
)
print({"status": "ok", "prefix_has_objects": bool(page.get("KeyCount"))})
PY
```

После передачи credentials установщику RDF-проекта удалите локальный `.env.rdf-reader` и при необходимости отзовите ключ. Для ротации выпустите новый ключ, передайте его безопасно, проверьте чтение и удалите старый ключ.

## Ссылки Yandex Cloud

- [Права на бакет через IAM](https://yandex.cloud/en/docs/storage/operations/buckets/iam-access)
- [ACL бакета и grant `READ`](https://yandex.cloud/en/docs/storage/operations/buckets/edit-acl)
- [Создание статического S3-ключа](https://yandex.cloud/en/docs/iam/operations/authentication/manage-access-keys)
- [Доступ к Object Storage через S3 API](https://yandex.cloud/en/docs/storage/s3/)
