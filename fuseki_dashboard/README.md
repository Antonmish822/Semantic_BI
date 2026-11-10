# Semantic BI Monitor

Read-only веб-монитор для синхронизации Bitrix24 из S3 в RDF-хранилище Fuseki. Он показывает состояние загрузчика и экспортёра, доступность Fuseki, число именованных графов по типам и самые свежие `modifiedAt`.

Главная страница обновляется каждые 20 секунд. API `GET /api/status` объединяет три источника:

- `status.json` загрузчика рядом с SQLite checkpoint;
- `state.json`, manifest и status последнего exporter run в S3;
- агрегаты именованных графов и `modifiedAt` из Fuseki.

SPARQL-шаблоны находятся в `queries/`. Дополнительный график по `b24:createdAt` отражает дату создания записи в Bitrix24, а не свежесть синхронизации. OWL 2 аксиома классификации активности описана в [ONTOLOGY.md](ONTOLOGY.md).

## Требования к среде

| Способ запуска | Требования |
|---|---|
| Локально | Linux/macOS, Python 3.10+, `uv` (проект проверен с 0.9.10), сетевой доступ к S3 и Fuseki |
| Docker Compose | Docker Engine и Compose plugin, доступ к Docker network Fuseki и checkpoint volume загрузчика |

Локальные Python-зависимости зафиксированы в `uv.lock`; основная зависимость приложения — `boto3`. В контейнере используется Python 3.13 и образ собирается по `deploy/monitor/Dockerfile`.

Для синхронизации с экспортёром требуется S3 credential с правами только на чтение/листинг нужного portal prefix. Для Fuseki используйте учётную запись только для чтения SPARQL, если такая настроена.

## Настройка

Создайте локальный файл конфигурации:

```bash
cd "/путь/к/bitrix24_dashboard/Semantic_BI/fuseki_dashboard"
umask 077
cp .env.example .env
chmod 600 .env
```

Заполните `.env`:

- `FUSEKI_URL`, `FUSEKI_USERNAME`, `FUSEKI_PASSWORD` — dataset и доступ к Fuseki;
- `GRAPH_PREFIX` — namespace графов с portal ID, например `urn:bitrix24:entity:garpix-bitrix24-ru:`;
- `S3_BUCKET`, `S3_PORTAL_ID`, `S3_ENDPOINT_URL`, `S3_REGION` и AWS credentials — доступ только для чтения к данным экспортёра;
- `LOADER_STATUS_PATH` — путь к `status.json` загрузчика.

Секреты остаются в `.env` на сервере и не включаются в браузер. Файл исключён из Git.

## Локальный запуск

Из каталога `fuseki_dashboard` выполните:

```bash
uv sync --locked
uv run --locked python server.py
```

Откройте <http://127.0.0.1:8080>. Для первого запуска используйте `uv lock`, если lock-файл ещё не создан. При отсутствии локального `status.json` монитор покажет состояние загрузчика как неизвестное, но продолжит показывать состояние S3 и Fuseki.

## Запуск в Docker Compose

В этом checkout уже подготовлены `.env`, внешний checkpoint volume `semantic_bi_loader_state_migrated` и сеть Fuseki `fuseki_default`. В `.env` должны быть права `600`, владелец — пользователь, запускающий Compose. Из корня `Semantic_BI` выполните:

```bash
cd "/путь/к/bitrix24_dashboard/Semantic_BI"
export LOADER_UID="$(id -u)" LOADER_GID="$(id -g)"
docker compose -p semantic_bi -f deploy/loader/compose.yaml up -d --build monitor
```

Эта команда собирает и запускает только сервис `monitor`; активный сервис `loader` не пересоздаётся. Интерфейс публикуется только на loopback хоста по адресу <http://127.0.0.1:8080>. Проверить журнал:

```bash
docker compose -p semantic_bi -f deploy/loader/compose.yaml logs -f monitor
```

Остановить только монитор, оставив загрузчик и checkpoint работать:

```bash
docker compose -p semantic_bi -f deploy/loader/compose.yaml stop monitor
```

Compose подключает монитор к S3 и сети Fuseki, а каталог `/var/lib/semantic-bi` монтирует в режиме read-only. Не удаляйте внешний checkpoint volume при остановке.

### Состояние загрузчика

Загрузчик пишет `status.json` рядом с checkpoint. Для его появления требуется образ загрузчика с поддержкой status-файла; уже работающий старый контейнер не создаёт его автоматически. После плановой пересборки и пересоздания загрузчика монитор увидит этот файл через общий volume. Пока файла нет, карточка синхронизации сообщает «нет данных», а состояние экспортёра и Fuseki остаётся доступно.

## Безопасность и доступ

Дашборд выполняет только SPARQL `SELECT`; он не меняет Fuseki и не пишет в S3. Веб-интерфейс не требует авторизации и по умолчанию слушает только `127.0.0.1`. Не задавайте `DASHBOARD_HOST=0.0.0.0` без сетевого ограничения или внешней авторизации.
