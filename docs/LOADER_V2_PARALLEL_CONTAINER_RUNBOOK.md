# Переход загрузчика на v2 через резервную копию основной станции

Основная станция уже работает на v2. На ней создаётся согласованный пакет GraphDB + SQLite checkpoint и публикуется в закрытый bucket Yandex Object Storage. На другой машине пакет скачивается, проверяется и восстанавливается в локальную GraphDB; затем запускаются контейнеры загрузчика и монитора `v2.0.0`. Машины соединяются с Yandex Cloud исходящими запросами.

Команды выполняются из каталога `Semantic_BI` **на указанной станции**. Для переноса используется `yc` CLI; для загрузки образов нужен доступ к GHCR. Образы `linux/amd64`: `ghcr.io/antonmish822/semantic-bi-loader:v2.0.0` и `ghcr.io/antonmish822/semantic-bi-monitor:v2.0.0`.

## 1. Основная станция: создать и опубликовать backup

Подготовьте закрытый bucket Yandex Object Storage **вне префикса событий загрузчика**. Для этой станции нужен профиль `yc` сервисного аккаунта с правом `storage.uploader` на bucket; для другой машины — отдельный профиль с `storage.viewer`. Ключи и конфигурация загрузчика хранятся вне Git. Профиль `yc` настраивается один раз:

```bash
yc config profile create semantic-bi-backup-primary
yc config set service-account-key /защищённый/путь/primary-key.json
yc config set cloud-id "<cloud-id>"
yc config set folder-id "<folder-id>"
```

Задайте локальные пути действующего v2 загрузчика и URI bucket. Имя bucket и ключи не записывайте в репозиторий:

```bash
export YC_PROFILE=semantic-bi-backup-primary
export BACKUP_BUCKET="<имя-закрытого-бакета>"
export BACKUP_PREFIX=semantic-bi/v2-parallel
export CONFIG_FILE="$PWD/local-data/v2-parallel/container-loader.yaml"
export STATE_DIR="$PWD/local-data/v2-parallel/container-state"
export BACKUP_DIR="$PWD/local-data/v2-parallel/container-backups"
export LOADER_IMAGE=ghcr.io/antonmish822/semantic-bi-loader:v2.0.0
mkdir -p "$BACKUP_DIR"
chmod 700 "$BACKUP_DIR"
```

Backup CLI запрашивает у работающего загрузчика паузу после цикла, сохраняет GraphDB и checkpoint в один пакет и возобновляет загрузчик:

```bash
docker run --rm --platform linux/amd64 --network host \
  --user "$(id -u):$(id -g)" --read-only --tmpfs /tmp \
  --mount "type=bind,src=$CONFIG_FILE,dst=/run/secrets/loader_config,readonly" \
  --mount "type=bind,src=$STATE_DIR,dst=/var/lib/semantic-bi" \
  --mount "type=bind,src=$BACKUP_DIR,dst=/backups" \
  --entrypoint python "$LOADER_IMAGE" \
  -m pipelines.backup.cli --config /run/secrets/loader_config create --output /backups
```

Из выведенного пути возьмите имя созданного каталога `backup-...`. Проверьте пакет и отправьте **весь каталог** в Yandex Object Storage:

```bash
export BACKUP_ID="backup-YYYYMMDDTHHMMSSZ-XXXXXXXX"
docker run --rm --platform linux/amd64 \
  --user "$(id -u):$(id -g)" --read-only --tmpfs /tmp \
  --mount "type=bind,src=$BACKUP_DIR,dst=/backups,readonly" \
  --entrypoint python "$LOADER_IMAGE" \
  -m pipelines.backup.cli verify "/backups/$BACKUP_ID"

yc --profile "$YC_PROFILE" storage s3 cp --recursive \
  "$BACKUP_DIR/$BACKUP_ID/" \
  "s3://$BACKUP_BUCKET/$BACKUP_PREFIX/$BACKUP_ID/"
```

Переходите дальше только после `valid: true` и успешного завершения `yc storage s3 cp`. Пакет содержит данные CRM, поэтому bucket должен быть приватным.

## 2. Другая машина: восстановить данные и запустить контейнеры v2

Обновите checkout `Semantic_BI` до версии, содержащей `deploy/loader/migrate_additional_station_v2.sh`. На машине должны работать локальная GraphDB с совместимой версией, старые загрузчик и монитор. Конфигурация загрузчика должна использовать **тот же URL и имя repository**, что на основной станции: например, `http://127.0.0.1:7200` и `bitrix24_v2_parallel` на каждой машине указывают на её собственную GraphDB. Restore проверяет точное совпадение GraphDB identity.

Один раз настройте `yc` с сервисным аккаунтом, которому разрешено читать закрытый backup bucket:

```bash
yc config profile create semantic-bi-backup-readonly
yc config set service-account-key /защищённый/путь/readonly-key.json
yc config set cloud-id "<cloud-id>"
yc config set folder-id "<folder-id>"
```

Подготовьте локальные конфигурации с правами `0600`:

- `local-data/v2-parallel/container-loader.yaml` — конфигурация v2 по `deploy/loader/config.example.yaml`: `backend: graphdb`, локальный GraphDB URL и repository, S3 bucket и portal ID источника, credentials, mappings, `checkpoint.path: /var/lib/semantic-bi/state.sqlite3`. Каталог `container-state` должен быть bind mount старого загрузчика и содержать `status.json` и `state.sqlite3`.
- `rdf_store_monitor/.env.graphdb` — конфигурация по `rdf_store_monitor/.env.example`: `RDF_BACKEND=graphdb`, `GRAPHDB_REPOSITORY` тот же, `RDF_URL` — адрес локальной GraphDB **из Docker network монитора** (например, `http://graphdb:7200`), параметры S3 и credentials для чтения. Путь к `status.json` скрипт задаёт сам.
- `deploy/loader/additional-station.env` — локальные пути, профиль `yc`, имя backup bucket, имена **работающих сейчас** контейнеров, Docker network GraphDB и порт монитора. Скопируйте образец и заполните его; имена контейнеров узнайте через `docker ps`:

```bash
mkdir -p local-data/v2-parallel
test -f local-data/v2-parallel/container-loader.yaml || \
  cp deploy/loader/config.example.yaml local-data/v2-parallel/container-loader.yaml
test -f rdf_store_monitor/.env.graphdb || \
  cp rdf_store_monitor/.env.example rdf_store_monitor/.env.graphdb
cp deploy/loader/additional-station.env.example deploy/loader/additional-station.env
chmod 600 deploy/loader/additional-station.env \
  local-data/v2-parallel/container-loader.yaml rdf_store_monitor/.env.graphdb
docker ps --format 'table {{.Names}}\t{{.Ports}}'
```

На основной машине возьмите `backup-...` из вывода команды создания копии. На дополнительной машине после заполнения файла настроек выполните **одну команду**:

```bash
bash deploy/loader/migrate_additional_station_v2.sh backup-YYYYMMDDTHHMMSSZ-XXXXXXXX
```

Скрипт скачивает пакет из Yandex Object Storage через `yc`, проверяет SHA-256 и backup CLI, скачивает оба образа `v2.0.0`, сверяет GraphDB identity, останавливает старый загрузчик на границе цикла, восстанавливает GraphDB и checkpoint вместе, создаёт и запускает новые контейнеры загрузчика и монитора. В конце он проверяет работу контейнеров и доступ монитора к загрузчику, GraphDB и S3; страница монитора — `http://127.0.0.1:8081/` (или заданный `MONITOR_PORT`). Старые контейнеры сохраняются с суффиксом `_previous_image`. Если восстановление прервётся, скрипт оставит старый загрузчик остановленным: сначала проверьте согласованность GraphDB и checkpoint.

Команды `yc storage s3 cp --recursive` и настройка профиля описаны в [документации Yandex Cloud CLI](https://yandex.cloud/en/docs/storage/tools/yc-cli).
