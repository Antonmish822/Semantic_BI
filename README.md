# Semantic_BI

Semantic_BI загружает данные Bitrix24 из S3 в RDF-хранилище и предоставляет их в виде именованных графов для семантической аналитики. Проект преобразует JSON-события экспортёра через CCCR mappings и поддерживает два backend: Apache Jena Fuseki и GraphDB.

Главный компонент — S3→RDF-загрузчик в `pipelines/bitrix24`. `raw_ingest` — отдельный коннектор для получения RAW-данных Bitrix24, а `rdf_store_monitor` — интерфейс чтения данных из выбранного RDF-хранилища (Fuseki или GraphDB).

Требования среды и инструкции локального и контейнерного запуска монитора приведены в [README RDF Store Monitor](rdf_store_monitor/README.md).

Для оперативного контроля активных сделок, бездействия по сделкам и нагрузки сотрудников есть локальный [CRM Pulse Monitor](crm_pulse_monitor/README.md). Он читает метрики из GraphDB или Fuseki и не изменяет RDF-хранилище.

Для параллельного переноса текущих именованных графов из Fuseki в GraphDB с отдельными загрузчиком, монитором и checkpoint см. [руководство GraphDB-миграции](docs/GRAPHDB_PARALLEL_MIGRATION.md).

Пошаговая команда-инструкция для подготовки repository, экспорта, переноса checkpoint, запуска и проверки отдельной пары контейнеров находится в [инструкции настройки GraphDB-связки](docs/GRAPHDB_PARALLEL_SETUP_INSTRUCTIONS.md).

Краткие правила работы ИИ-агента с GraphDB API собраны в [памятке GraphDB API](docs/GRAPHDB_AGENT_API_GUIDE.md).

Согласованный backup/restore RDF-хранилища и loader checkpoint для GraphDB и Fuseki описан в [архитектуре и инструкции резервного копирования](docs/BACKUP_RESTORE_ARCHITECTURE.md). Инструкция включает онлайн pause barrier и offline-режим для старых образов. Для Fuseki используйте [deploy/fuseki/compose.yaml](deploy/fuseki/compose.yaml) и локальную конфигурацию на основе [config.example.yaml](deploy/fuseki/config.example.yaml).

## Что делает загрузчик

Для каждой CRM-сущности загрузчик вычисляет стабильный URI именованного графа по `entity_key`. Новая версия сущности полностью заменяет её граф; событие удаления удаляет граф. SQLite checkpoint хранит принятый snapshot, позиции обработки и применённые версии mappings. Checkpoint должен сохраняться между запусками и использоваться только с тем RDF-хранилищем, которому он принадлежит.

### Разовый цикл

Запуск с `--once` выполняет один цикл и завершает процесс. При пустом checkpoint загрузчик применяет принятый полный snapshot, сверяет сущности только для настроенных полных типов и затем догоняет завершённые incremental runs до курсора из `state.json`. Это режим одного цикла, а не отдельный режим «только первичная загрузка».

### Инкрементальная обработка

При последующих циклах загрузчик проверяет состояние snapshot и выбирает incremental manifests после своего checkpoint до верхней границы `state.json`. Он обрабатывает только завершённые запуски с успешным `status.json`, сверяет SHA-256 событий и проверяет их идентичность. `upsert` заменяет граф через Graph Store `PUT`, `delete` удаляет граф. После успешной обработки сущностей и запусков позиции сохраняются в SQLite. Если процесс перезапущен, безопасный повтор продолжает обработку с checkpoint.

Без `--once` загрузчик выполняет цикл регулярно с интервалом `poll_interval_seconds`. Подробности контракта и восстановления описаны в [RDF_INTEGRATION.md](docs/RDF_INTEGRATION.md) и [руководстве по загрузке](docs/S3_TO_FUSEKI_LOADING.md); известные ограничения собраны в [reliability backlog](docs/SYNC_RELIABILITY_BACKLOG.md).

## Развёртывание

Имена GHCR-образов, правила публикации и закрепления версий описаны в [контракте контейнерных образов](docs/CONTAINER_IMAGE_CONTRACT.md). GitHub Actions публикует loader и monitor; на сервере остаётся только скачать и запустить готовые образы.

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

Заполните в `deploy/loader/config.yaml` S3 bucket и ключи, выбранный backend и адрес хранилища. Для Fuseki `fuseki.url` задаётся как адрес dataset без `/data`; для GraphDB задайте `graphdb.url` как адрес сервера без `/repositories` и имя `graphdb.repository`. Пути `/app/mappings/bitrix24/...` подходят для контейнера; для локального запуска замените их на пути checkout, задайте адрес RDF-хранилища, доступный с хоста, и отдельный локальный `checkpoint.path`. Для GraphDB используйте отдельный checkpoint и том. По умолчанию `checkpoint.resume_mode: strict` скачивает и проверяет SHA-256 каждого события. Для ускорения повторного запуска после прерывания задайте `checkpoint.resume_mode: fast`; режим пропустит скачивание только для событий, которые точно совпадают с checkpoint. Описание и ограничения режима — в [руководстве по загрузке](docs/S3_TO_FUSEKI_LOADING.md#быстрое-восстановление). Не добавляйте в Git конфигурацию с секретами или файлы SQLite.

### 4. Получить опубликованные образы

CI собирает loader вместе с native `cccr_engine` и monitor, затем публикует образы `linux/amd64`. Они запускаются на Linux и в Docker Desktop на Windows/WSL2 и macOS; на Apple Silicon используется amd64-эмуляция. Нативный arm64 пока не поддержан. На целевой машине исходники и компилятор не нужны. Для закрытых GHCR packages войдите с токеном `read:packages`:

```bash
read -rsp 'GitHub token: ' GHCR_TOKEN; echo
printf '%s' "$GHCR_TOKEN" | docker login ghcr.io -u ВАШ_GITHUB_ЛОГИН --password-stdin
unset GHCR_TOKEN
```

Используйте GitHub Personal Access Token (classic) с правом `read:packages`. `read -s` скрывает ввод токена; не передавайте токен аргументом команды и не сохраняйте его в конфигурации проекта.

Укажите один и тот же опубликованный тег для обоих образов. Последний Git-тег в этом checkout — `v1.3.0`; используйте его, если workflow публикации для этого тега завершился успешно и образы доступны в GHCR:

```bash
export LOADER_IMAGE_TAG=v1.3.0
export MONITOR_IMAGE_TAG=v1.3.0
docker compose -p semantic_bi -f deploy/loader/compose.yaml pull
```

Workflow публикует образы только при push Git-тега `v*`; push в ветки запускает проверки, но не публикацию. Для точного выбора можно вместо version tag указать `sha-<полный-git-sha>`. Compose требует явно задать обе переменные и не использует подвижные branch-теги. Выполняйте `pull` перед первым запуском и после смены тега. Конфигурация с credentials остаётся в `deploy/loader/config.yaml`, отдельно от образа; checkpoint хранится в постоянном volume.

#### UID и GID контейнеров

Compose задаёт пользователю loader и monitor значения `LOADER_UID:LOADER_GID`. Это позволяет читать file-backed secrets с правами `600` и работать с checkpoint volume. Если переменные не заданы, Compose использует `1000:1000`: на Linux с таким UID/GID экспорт необязателен; если значения отличаются, задайте UID/GID владельца файлов и используйте их при последующих запусках с тем же volume. Сначала настройте права на конфиги, например `chmod 600 deploy/loader/config.yaml rdf_store_monitor/.env`, затем перед командами Compose выполните в Linux, macOS или WSL:

```bash
export LOADER_UID="$(id -u)"
export LOADER_GID="$(id -g)"
```

На Windows запускайте эти команды из WSL (в PowerShell `id` обычно отсутствует). Используйте те же UID/GID при каждом обновлении или перезапуске сервисов: смена пользователя может лишить контейнер доступа к уже созданному checkpoint volume. Если `id -u` и `id -g` оба возвращают `1000`, явный экспорт просто совпадает со значениями Compose по умолчанию.

#### Запуск опубликованных контейнеров

На чистом сервере, из корня checkout, создайте локальные файлы конфигурации. Не выполняйте `cp`, если файлы уже настроены: команда перезапишет их примерами.

```bash
umask 077
cp deploy/loader/config.example.yaml deploy/loader/config.yaml
cp rdf_store_monitor/.env.example rdf_store_monitor/.env
chmod 600 deploy/loader/config.yaml rdf_store_monitor/.env
```

Заполните оба файла. В loader config укажите S3 credentials и backend. В `.env` монитора укажите S3 credentials и адрес RDF-хранилища, доступный из Docker-сети (не `127.0.0.1` контейнера); установите `LOADER_STATUS_PATH=/var/lib/semantic-bi/status.json`. Убедитесь, что Docker-сеть RDF-хранилища существует. Fuseki-сеть по умолчанию — `fuseki_default`; для GraphDB задайте `RDF_DOCKER_NETWORK` равным имени его сети. Compose использует внешний volume `semantic_bi_loader_state_migrated`: если это новая установка без checkpoint, создайте пустой volume; для продолжения существующей загрузки используйте уже имеющийся volume с checkpoint.

Пример полного запуска для последнего Git-тега этого checkout (`v1.3.0`). Используйте этот тег, если он есть в GHCR и workflow его публикации завершился успешно; иначе подставьте опубликованный тег со страницы пакета. Следующие команды рассчитаны на Linux/macOS/WSL:

```bash
# GHCR packages закрыты: токен должен иметь read:packages.
read -rsp 'GitHub token: ' GHCR_TOKEN; echo
printf '%s' "$GHCR_TOKEN" | docker login ghcr.io -u ВАШ_GITHUB_ЛОГИН --password-stdin
unset GHCR_TOKEN

export LOADER_IMAGE_TAG=v1.3.0
export MONITOR_IMAGE_TAG=v1.3.0
export LOADER_UID="$(id -u)"
export LOADER_GID="$(id -g)"

# Только для новой установки без существующего checkpoint:
docker volume create semantic_bi_loader_state_migrated

docker compose -p semantic_bi -f deploy/loader/compose.yaml pull
docker compose -p semantic_bi -f deploy/loader/compose.yaml up -d
docker compose -p semantic_bi -f deploy/loader/compose.yaml ps
docker compose -p semantic_bi -f deploy/loader/compose.yaml logs -f loader monitor
```

UID/GID задают пользователя, от имени которого работают оба контейнера. Compose по умолчанию использует `1000:1000`: если `id -u` и `id -g` равны 1000, экспорт можно опустить; иначе подставьте UID/GID пользователя, которому принадлежат конфиги и checkpoint. Оставляйте те же значения при последующих перезапусках с этим volume. В Windows запускайте пример из WSL; в PowerShell команды `id` нет.

Монитор слушает `http://127.0.0.1:8080`. Остановить оба контейнера, сохранив checkpoint:

```bash
docker compose -p semantic_bi -f deploy/loader/compose.yaml down
```

### 5. Проверить один цикл

Локально:

```bash
uv run python -m pipelines.bitrix24.cli \
  --config deploy/loader/config.yaml --once
```

В контейнере:

```bash
export LOADER_UID="$(id -u)" LOADER_GID="$(id -g)"
docker compose -f deploy/loader/compose.yaml run --rm loader \
  --config /run/secrets/loader_config --once
```

### 6. Запустить регулярную обработку

```bash
docker compose -p semantic_bi -f deploy/loader/compose.yaml up -d
docker compose -f deploy/loader/compose.yaml logs -f loader
```

После выбора нового SHA-тега скачайте образы и пересоздайте нужные сервисы. Checkpoint остаётся в томе:

```bash
docker compose -p semantic_bi -f deploy/loader/compose.yaml pull loader
docker compose -p semantic_bi -f deploy/loader/compose.yaml up -d --no-deps loader
```

Чтобы обновить только монитор, используйте `pull monitor` и `up -d --no-deps monitor`. Откат выполняется выбором предыдущего `sha-...` для соответствующего `*_IMAGE_TAG` и повторным `pull`/`up -d`.

Compose монтирует конфигурацию как secret и хранит checkpoint в постоянном томе `loader_state`. Для продолжения после перезапуска сохраняйте этот том и тот же dataset/repository. Перед переносом checkpoint проверяйте его на копии. Не запускайте одновременно два загрузчика с одним checkpoint. Старый checkpoint без `store_backend` принимается только для Fuseki и при первом старте привязывается к выбранному endpoint. Для GraphDB создайте отдельный checkpoint и выполните начальную загрузку из принятого полного snapshot. Смена backend сама по себе не переносит RDF-графы: перед переключением сверьте состав графов в целевом хранилище.

Для отдельного GraphDB loader volume запускайте Compose с другим project name:

```bash
docker compose -p semantic-bi-graphdb \
  -f deploy/loader/compose.yaml up -d
```

Используйте конфигурацию, в которой `backend: graphdb`, а `graphdb.repository` указывает на созданный repository.

Подготовка и запуск загрузчика как непосредственно в терминале, так и в Docker Compose, включая однократный и постоянный режимы, описаны в [единой инструкции запуска](docs/SYNC_RUNBOOK.md).

Остановить загрузчик можно без удаления checkpoint:

```bash
docker compose -f deploy/loader/compose.yaml down
```

Не добавляйте `-v`, если нужно сохранить том `loader_state`.

Для настройки GraphDB и правил миграции см. [план рефакторинга](docs/plans/ARCHITECTURE_REFACTOR_PLAN.md) и [требования к проверке](docs/plans/testing_requirements.md).
