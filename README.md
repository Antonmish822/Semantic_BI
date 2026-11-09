# Semantic_BI

Semantic_BI загружает данные Bitrix24 из S3 в RDF-хранилище и предоставляет их в виде именованных графов для семантической аналитики. Проект преобразует JSON-события экспортёра через CCCR mappings и поддерживает два backend: Apache Jena Fuseki и GraphDB.

Главный компонент — S3→RDF-загрузчик в `pipelines/bitrix24`. `raw_ingest` — отдельный коннектор для получения RAW-данных Bitrix24, а `fuseki_dashboard` — интерфейс чтения данных из Fuseki.

## Что делает загрузчик

Для каждой CRM-сущности загрузчик вычисляет стабильный URI именованного графа по `entity_key`. Новая версия сущности полностью заменяет её граф; событие удаления удаляет граф. SQLite checkpoint хранит принятый snapshot, позиции обработки и применённые версии mappings. Checkpoint должен сохраняться между запусками и использоваться только с тем RDF-хранилищем, которому он принадлежит.

### Разовый цикл

Запуск с `--once` выполняет один цикл и завершает процесс. При пустом checkpoint загрузчик применяет принятый полный snapshot, сверяет сущности только для настроенных полных типов и затем догоняет завершённые incremental runs до курсора из `state.json`. Это режим одного цикла, а не отдельный режим «только первичная загрузка».

### Инкрементальная обработка

При последующих циклах загрузчик проверяет состояние snapshot и выбирает incremental manifests после своего checkpoint до верхней границы `state.json`. Он обрабатывает только завершённые запуски с успешным `status.json`, сверяет SHA-256 событий и проверяет их идентичность. `upsert` заменяет граф через Graph Store `PUT`, `delete` удаляет граф. После успешной обработки сущностей и запусков позиции сохраняются в SQLite. Если процесс перезапущен, безопасный повтор продолжает обработку с checkpoint.

Без `--once` загрузчик выполняет цикл регулярно с интервалом `poll_interval_seconds`. Подробности контракта и восстановления описаны в [RDF_INTEGRATION.md](docs/RDF_INTEGRATION.md) и [руководстве по загрузке](docs/S3_TO_FUSEKI_LOADING.md); известные ограничения собраны в [reliability backlog](docs/SYNC_RELIABILITY_BACKLOG.md).

## Развёртывание

### 1. Получить проект и зависимости

```bash
git clone --recurse-submodules <URL-Semantic_BI>
cd Semantic_BI
uv sync --locked
uv run python -m unittest discover -s tests -v
```

Для сборки native `cccr_py` нужны системные зависимости, перечисленные в [Dockerfile загрузчика](deploy/loader/Dockerfile). CI получает доступ к закрытому submodule `cccr_engine` через `SOURCECRAFT_SSH_KEY` и `SOURCECRAFT_KNOWN_HOSTS`.

### 2. Подготовить RDF-хранилище

До запуска загрузчика создайте dataset в Fuseki или repository в GraphDB и проверьте адрес и учётные данные. Загрузчик подключается к уже запущенному хранилищу; Compose в `deploy/loader/compose.yaml` поднимает только загрузчик.

Для локальной демонстрации Fuseki используйте [`examples/dev/docker-compose.fuseki.yaml`](examples/dev/docker-compose.fuseki.yaml): `docker compose -f examples/dev/docker-compose.fuseki.yaml up -d`. Это конфигурация с in-memory dataset. Она не сохраняет RDF между перезапусками и не предназначена для рабочего запуска. Для постоянных данных используйте настроенный TDB2 dataset или управляемый GraphDB repository.

### 3. Настроить загрузчик

```bash
cp deploy/loader/config.example.yaml deploy/loader/config.yaml
```

Заполните в `deploy/loader/config.yaml` S3 bucket и ключи, выбранный backend и адрес хранилища. Для Fuseki `fuseki.url` задаётся как адрес dataset без `/data`; для GraphDB задайте `graphdb.url` как адрес сервера без `/repositories` и имя `graphdb.repository`. Пути `/app/mappings/bitrix24/...` подходят для контейнера; для локального запуска замените их на пути checkout, задайте адрес RDF-хранилища, доступный с хоста, и отдельный локальный `checkpoint.path`. Для GraphDB используйте отдельный checkpoint и том. Не добавляйте в Git конфигурацию с секретами или файлы SQLite.

### 4. Проверить один цикл

Локально:

```bash
uv run python -m pipelines.bitrix24.cli \
  --config deploy/loader/config.yaml --once
```

В контейнере после сборки:

```bash
docker build --platform linux/amd64 \
  -f deploy/loader/Dockerfile -t semantic-bi-loader:local .
docker compose -f deploy/loader/compose.yaml run --rm loader \
  --config /run/secrets/loader_config --once
```

### 5. Запустить регулярную обработку

```bash
docker compose -f deploy/loader/compose.yaml up -d --build
docker compose -f deploy/loader/compose.yaml logs -f loader
```

Compose монтирует конфигурацию как secret и хранит checkpoint в постоянном томе `loader_state`. Для продолжения после перезапуска сохраняйте этот том и тот же dataset/repository. Перед переносом checkpoint проверяйте его на копии. Не запускайте одновременно два загрузчика с одним checkpoint. Старый checkpoint без `store_backend` принимается только для Fuseki и при первом старте привязывается к выбранному endpoint. Для GraphDB создайте отдельный checkpoint и выполните начальную загрузку из принятого полного snapshot. Смена backend сама по себе не переносит RDF-графы: перед переключением сверьте состав графов в целевом хранилище.

Для отдельного GraphDB loader volume запускайте Compose с другим project name:

```bash
docker compose -p semantic-bi-graphdb \
  -f deploy/loader/compose.yaml up -d --build
```

Используйте конфигурацию, в которой `backend: graphdb`, а `graphdb.repository` указывает на созданный repository.

Остановить загрузчик можно без удаления checkpoint:

```bash
docker compose -f deploy/loader/compose.yaml down
```

Не добавляйте `-v`, если нужно сохранить том `loader_state`.

Для настройки GraphDB и правил миграции см. [план рефакторинга](docs/plans/ARCHITECTURE_REFACTOR_PLAN.md) и [требования к проверке](docs/plans/testing_requirements.md).
