#!/usr/bin/env bash
set -Eeuo pipefail

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
SETTINGS_FILE="$PROJECT_DIR/deploy/loader/additional-station.env"
LOADER_IMAGE=ghcr.io/antonmish822/semantic-bi-loader:v2.0.0
MONITOR_IMAGE=ghcr.io/antonmish822/semantic-bi-monitor:v2.0.0
LOADER_NAME=semantic_bi_v2_parallel_loader
MONITOR_NAME=semantic_bi_v2_parallel_monitor
STOPPED=0

on_error() {
  if (( STOPPED )); then
    printf 'Переход прерван после остановки старых контейнеров. Не запускайте старый загрузчик до проверки пары GraphDB/checkpoint.\n' >&2
  fi
}
fail() { printf 'Ошибка: %s\n' "$*" >&2; on_error; exit 1; }
step() { printf '\n==> %s\n' "$*"; }
trap on_error ERR

[[ $# -eq 1 ]] || fail "Использование: $0 backup-YYYYMMDDTHHMMSSZ-XXXXXXXX"
BACKUP_ID=$1
[[ $BACKUP_ID =~ ^backup-[0-9]{8}T[0-9]{6}Z-[0-9a-f]{8}$ ]] || fail "Некорректный backup ID: $BACKUP_ID"
[[ -f $SETTINGS_FILE ]] || fail "Создайте $SETTINGS_FILE из additional-station.env.example"
# The settings file belongs to the local operator and is never committed.
# shellcheck source=/dev/null
source "$SETTINGS_FILE"

for name in YC_PROFILE BACKUP_BUCKET BACKUP_PREFIX CONFIG_FILE STATE_DIR BACKUP_DIR MONITOR_ENV_FILE OLD_LOADER_NAME OLD_MONITOR_NAME MONITOR_NETWORK MONITOR_PORT; do
  [[ -n ${!name:-} && ${!name} != *'<'* ]] || fail "Заполните $name в $SETTINGS_FILE"
done
[[ $BACKUP_PREFIX =~ ^[A-Za-z0-9._/-]+$ && $BACKUP_PREFIX != *..* ]] || fail "Некорректный BACKUP_PREFIX"
[[ $MONITOR_PORT =~ ^[0-9]+$ ]] && (( MONITOR_PORT >= 1 && MONITOR_PORT <= 65535 )) || fail "Некорректный MONITOR_PORT"
[[ $OLD_LOADER_NAME != "$OLD_MONITOR_NAME" ]] || fail "Имена старых контейнеров должны различаться"
for command in yc docker python3 curl sha256sum realpath; do
  command -v "$command" >/dev/null || fail "Не найдена команда $command"
done

[[ -f $CONFIG_FILE ]] || fail "Не найден config загрузчика: $CONFIG_FILE"
[[ -f $MONITOR_ENV_FILE ]] || fail "Не найден config монитора: $MONITOR_ENV_FILE"
[[ -f $STATE_DIR/status.json && -f $STATE_DIR/state.sqlite3 ]] || fail "В STATE_DIR нужны status.json и state.sqlite3"
CONFIG_FILE=$(realpath "$CONFIG_FILE")
STATE_DIR=$(realpath "$STATE_DIR")
MONITOR_ENV_FILE=$(realpath "$MONITOR_ENV_FILE")
mkdir -p "$BACKUP_DIR"
BACKUP_DIR=$(realpath "$BACKUP_DIR")
chmod 700 "$BACKUP_DIR"
chmod 600 "$CONFIG_FILE" "$MONITOR_ENV_FILE"
docker network inspect "$MONITOR_NETWORK" >/dev/null || fail "Не найдена сеть GraphDB: $MONITOR_NETWORK"
for name in "$OLD_LOADER_NAME" "$OLD_MONITOR_NAME"; do
  docker container inspect "$name" >/dev/null || fail "Не найден контейнер $name"
  [[ $(docker inspect --format '{{.State.Running}}' "$name") == true ]] || fail "Контейнер $name не запущен"
  ! docker container inspect "${name}_previous_image" >/dev/null 2>&1 || fail "Уже существует ${name}_previous_image"
done
[[ $(docker inspect --format '{{.Config.Image}}' "$OLD_LOADER_NAME") != "$LOADER_IMAGE" ]] || fail "Загрузчик уже использует образ v2.0.0"
[[ $(docker inspect --format '{{.Config.Image}}' "$OLD_MONITOR_NAME") != "$MONITOR_IMAGE" ]] || fail "Монитор уже использует образ v2.0.0"
for name in "$LOADER_NAME" "$MONITOR_NAME"; do
  if [[ $name != "$OLD_LOADER_NAME" && $name != "$OLD_MONITOR_NAME" ]]; then
    ! docker container inspect "$name" >/dev/null 2>&1 || fail "Имя нового контейнера уже занято: $name"
  fi
done
old_state_dir=$(docker inspect --format '{{range .Mounts}}{{if eq .Destination "/var/lib/semantic-bi"}}{{.Source}}{{end}}{{end}}' "$OLD_LOADER_NAME")
[[ -n $old_state_dir && $(realpath "$old_state_dir") == "$STATE_DIR" ]] || fail "STATE_DIR не совпадает с mount старого загрузчика"

safe_status() {
  python3 - "$STATE_DIR/status.json" <<'PY'
import json, sys
with open(sys.argv[1], encoding="utf-8") as source:
    state = json.load(source).get("state")
print("loader_state=" + str(state))
if state not in {"waiting", "idle"}:
    raise SystemExit("Загрузчик должен находиться на границе цикла: waiting или idle")
PY
}

step "Скачивание и проверка backup $BACKUP_ID"
mkdir -p "$BACKUP_DIR/$BACKUP_ID"
chmod 700 "$BACKUP_DIR/$BACKUP_ID"
yc --profile "$YC_PROFILE" storage s3 cp --recursive \
  "s3://$BACKUP_BUCKET/$BACKUP_PREFIX/$BACKUP_ID/" "$BACKUP_DIR/$BACKUP_ID/"
(cd "$BACKUP_DIR/$BACKUP_ID" && sha256sum -c SHA256SUMS)

step "Скачивание образов v2"
docker pull --platform linux/amd64 "$LOADER_IMAGE"
docker pull --platform linux/amd64 "$MONITOR_IMAGE"
docker run --rm --platform linux/amd64 \
  --user "$(id -u):$(id -g)" --read-only --tmpfs /tmp \
  --mount "type=bind,src=$BACKUP_DIR,dst=/backups,readonly" \
  --entrypoint python "$LOADER_IMAGE" \
  -m pipelines.backup.cli verify "/backups/$BACKUP_ID"

# Check the target and exact GraphDB identity before stopping either container.
repository=$(docker run --rm -i --platform linux/amd64 \
  --user "$(id -u):$(id -g)" --read-only --tmpfs /tmp \
  --mount "type=bind,src=$CONFIG_FILE,dst=/run/secrets/loader_config,readonly" \
  --mount "type=bind,src=$BACKUP_DIR/$BACKUP_ID/manifest.json,dst=/run/backup_manifest.json,readonly" \
  --entrypoint python "$LOADER_IMAGE" - <<'PY'
import json
from pathlib import Path
from pipelines.backup.cli import safe_url
from pipelines.bitrix24.cli import make_store
from pipelines.bitrix24.config import load_config

config = load_config(Path("/run/secrets/loader_config"))
manifest = json.loads(Path("/run/backup_manifest.json").read_text(encoding="utf-8"))
if config.backend != "graphdb":
    raise SystemExit("Нужен backend: graphdb")
if config.checkpoint_path != Path("/var/lib/semantic-bi/state.sqlite3"):
    raise SystemExit("Путь checkpoint не совпадает с mount")
if manifest["store"]["target"] != config.graphdb_repository:
    raise SystemExit("Repository не совпадает с backup")
if manifest["store"]["identity"] != safe_url(make_store(config).endpoint):
    raise SystemExit("GraphDB identity не совпадает с backup")
print(config.graphdb_repository)
PY
)
python3 - "$MONITOR_ENV_FILE" "$repository" <<'PY'
import sys
settings = {}
with open(sys.argv[1], encoding="utf-8") as source:
    for line in source:
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            settings[key.strip()] = value.strip().strip("\"'")
if settings.get("RDF_BACKEND") != "graphdb":
    raise SystemExit("Монитор должен использовать GraphDB")
if settings.get("GRAPHDB_REPOSITORY") != sys.argv[2]:
    raise SystemExit("Repository монитора не совпадает")
if not settings.get("RDF_URL"):
    raise SystemExit("Не задан RDF_URL монитора")
if not settings.get("S3_BUCKET") or not settings.get("S3_PORTAL_ID"):
    raise SystemExit("Не настроен S3 монитора")
PY
safe_status

step "Остановка старого загрузчика на границе цикла"
old_loader_restart=$(docker inspect --format '{{.HostConfig.RestartPolicy.Name}}' "$OLD_LOADER_NAME")
docker update --restart=no "$OLD_LOADER_NAME"
docker stop -t 30 "$OLD_LOADER_NAME"
if ! safe_status; then
  docker update --restart="$old_loader_restart" "$OLD_LOADER_NAME"
  docker start "$OLD_LOADER_NAME"
  fail "Загрузчик был остановлен посреди цикла; старый контейнер запущен снова"
fi
STOPPED=1
docker update --restart=no "$OLD_MONITOR_NAME"
docker stop -t 15 "$OLD_MONITOR_NAME"

step "Совместное восстановление GraphDB и checkpoint"
docker run --rm --platform linux/amd64 --network host \
  --user "$(id -u):$(id -g)" --read-only --tmpfs /tmp \
  --mount "type=bind,src=$CONFIG_FILE,dst=/run/secrets/loader_config,readonly" \
  --mount "type=bind,src=$STATE_DIR,dst=/var/lib/semantic-bi" \
  --mount "type=bind,src=$BACKUP_DIR,dst=/backups,readonly" \
  --entrypoint python "$LOADER_IMAGE" \
  -m pipelines.backup.cli --config /run/secrets/loader_config \
  restore "/backups/$BACKUP_ID" --confirm-repository "$repository" --offline

step "Создание и запуск загрузчика v2"
docker rename "$OLD_LOADER_NAME" "${OLD_LOADER_NAME}_previous_image"
docker create --name "$LOADER_NAME" --restart unless-stopped \
  --platform linux/amd64 --network host \
  --user "$(id -u):$(id -g)" --read-only --tmpfs /tmp \
  --mount "type=bind,src=$CONFIG_FILE,dst=/run/secrets/loader_config,readonly" \
  --mount "type=bind,src=$STATE_DIR,dst=/var/lib/semantic-bi" \
  "$LOADER_IMAGE" --config /run/secrets/loader_config --log-level WARNING
docker start "$LOADER_NAME"

step "Создание и запуск монитора v2"
docker rename "$OLD_MONITOR_NAME" "${OLD_MONITOR_NAME}_previous_image"
docker create --name "$MONITOR_NAME" --restart unless-stopped \
  --platform linux/amd64 --network "$MONITOR_NETWORK" \
  --user "$(id -u):$(id -g)" --read-only --tmpfs /tmp \
  --env DASHBOARD_ENV_FILE=/run/secrets/dashboard_env \
  --env DASHBOARD_HOST=0.0.0.0 --env DASHBOARD_PORT=8080 \
  --env LOADER_STATUS_PATH=/var/lib/semantic-bi/status.json \
  --mount "type=bind,src=$MONITOR_ENV_FILE,dst=/run/secrets/dashboard_env,readonly" \
  --mount "type=bind,src=$STATE_DIR,dst=/var/lib/semantic-bi,readonly" \
  --publish "127.0.0.1:$MONITOR_PORT:8080" "$MONITOR_IMAGE"
docker start "$MONITOR_NAME"

step "Проверка контейнеров и монитора"
for name in "$LOADER_NAME" "$MONITOR_NAME"; do
  [[ $(docker inspect --format '{{.State.Running}}' "$name") == true ]] || fail "Контейнер $name остановился"
  docker inspect --format 'name={{.Name}} state={{.State.Status}} restarts={{.RestartCount}} image={{.Config.Image}}' "$name"
done
for attempt in 1 2 3 4 5 6 7 8 9 10; do
  if curl -fsS --max-time 10 "http://127.0.0.1:$MONITOR_PORT/api/status" 2>/dev/null | python3 -c '
import json, sys
data = json.load(sys.stdin)
if data["sync"].get("contract_version") != 2:
    raise SystemExit("Статус загрузчика ещё не обновлён до v2")
if not data["store"].get("available"):
    raise SystemExit("GraphDB недоступна монитору")
if data["exporter"].get("state") != "available":
    raise SystemExit("S3 недоступен монитору")
print("monitor=ok loader_state=" + str(data["sync"].get("state")))
' 2>/dev/null; then
    STOPPED=0
    printf 'Переход завершён. Монитор: http://127.0.0.1:%s/\n' "$MONITOR_PORT"
    exit 0
  fi
  sleep 3
done
fail "Монитор не подтвердил доступность загрузчика, GraphDB и S3; проверьте docker logs $MONITOR_NAME"
