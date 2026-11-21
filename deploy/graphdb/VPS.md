# Runbook Codex CLI: чистое развёртывание Semantic_BI на VPS

Этот runbook предназначен для Codex CLI, запущенного на Linux amd64 VPS из корня checkout `Semantic_BI`. Агент выполняет шаги по порядку, не выводит содержимое секретных файлов, не удаляет неизвестные Docker-ресурсы и не запускает постоянные сервисы до приёмки первого цикла.

Развёртывание состоит из двух Compose-проектов:

- `semantic-bi-graphdb`: GraphDB 11.5.2, repository `bitrix24`, сеть `graphdb_default`, RDF volume `semantic_bi_graphdb_home`;
- `semantic-bi-runtime`: loader и monitor, общая сеть с GraphDB, новый checkpoint/status volume `semantic_bi_graphdb_state`.

Локальные RDF-данные и SQLite на VPS не переносятся. Начальная загрузка выполняется одним strict циклом из полного S3 v2 snapshot и доступных инкрементов.

## 1. Подготовка и передача на VPS

Эти действия выполняет оператор на доверенной рабочей машине до запуска Codex CLI на VPS.

### 1.1 Подготовить закрытый bundle

Создайте GHCR-only Docker config на рабочей машине. Используйте отдельный Docker config directory и PAT classic с `read:packages`; PAT вводится только на этой машине. В текущей связке проверены манифесты обоих пакетов с тегом `v2.0.1`.

```sh
umask 077
ghcr_config_dir=$(mktemp -d /tmp/semantic-bi-ghcr-config.XXXXXX)
chmod 700 "$ghcr_config_dir"
read -rp 'GitHub username: ' ghcr_user
read -rsp 'GitHub PAT classic (read:packages): ' ghcr_token
printf '\n'
printf '%s' "$ghcr_token" | DOCKER_CONFIG="$ghcr_config_dir" docker login ghcr.io --username "$ghcr_user" --password-stdin
unset ghcr_user ghcr_token
```

Docker CLI записывает auth config в `$ghcr_config_dir/config.json`. PAT поступает в Docker CLI через stdin, не включается в командную строку или prompt Codex.

Затем из корня checkout выполните генератор bundle. Он читает действующий локальный S3 `.env`, GHCR config и лицензию; создает готовые `loader.yaml`, `monitor.env`, `graphdb.env`, `runtime.env`, `docker-config/config.json` и копию лицензии. Архив записывается вне checkout с правами `600`; генератор не печатает значения секретов и откажется писать архив внутрь репозитория.

```sh
python3 scripts/prepare_vps_bundle.py \
  --license-file /path/to/graphdb.license \
  --docker-config "$ghcr_config_dir/config.json" \
  --image-tag v2.0.1 \
  --output /tmp/semantic-bi-vps-configs.tar.gz
```

После создания архива удалите временный каталог Docker config, когда он больше не нужен: `rm -rf "$ghcr_config_dir"`.

`--source-env` по умолчанию указывает на `local-data/remote-graphdb-v2-20261115/graphdb-v2-parallel-2/.env` активной локальной связки. Скрипт сверяет bucket, portal ID, endpoint и region с tracked templates, переносит access key и secret key только в bundle. В нём также будут зафиксированы `GRAPHDB_REPOSITORY=bitrix24`, `http://graphdb:7200`, строгий checkpoint, `/var/lib/semantic-bi/status.json` и одинаковый image tag для loader/monitor.

Проверьте только имена файлов и права, не распечатывайте содержимое:

```sh
tar -tzf /tmp/semantic-bi-vps-configs.tar.gz
stat -c '%a %n' /tmp/semantic-bi-vps-configs.tar.gz
```

Передайте bundle по SSH:

```sh
scp /tmp/semantic-bi-vps-configs.tar.gz deploy@VPS:/tmp/semantic-bi-vps-configs.tar.gz
```

Архив содержит S3 credentials, GHCR auth config и лицензию GraphDB. Docker config содержит обратимо закодированные credentials и должен считаться секретом. Не добавляйте архив или распакованные файлы в Git. После успешной установки удалите переданные временные файлы на обеих машинах.

### 1.2 Подготовить checkout и запустить Codex CLI

После согласования и публикации deployment commit получите именно его на VPS. Секретный bundle передается отдельно:

```sh
git clone <URL-Semantic_BI> /opt/Semantic_BI
cd /opt/Semantic_BI
git checkout <deployment-commit-sha>
```

Запустите Codex CLI из `/opt/Semantic_BI` и поручите ему выполнить этот runbook. Не передавайте секреты в prompt: агент должен читать bundle с диска и использовать значения через файлы Compose.

## 2. Инструкции Codex CLI на сервере

### 2.1 Preflight: checkout, ресурсы и закрытый bundle

Проверьте корень репозитория, commit, архитектуру и доступность Docker:

```sh
pwd
git rev-parse --show-toplevel
git rev-parse HEAD
uname -m
docker info --format '{{.ServerVersion}}'
df -h /
```

Ожидаются корень `/opt/Semantic_BI`, архитектура `x86_64`, доступный Docker daemon и достаточное свободное место для RDF. Не выводите содержимое bundle или конфигов.

До создания ресурсов проверьте, что имена проекта, сеть, тома и порты свободны: Compose-проекты `semantic-bi-graphdb` и `semantic-bi-runtime`, сеть `graphdb_default`, тома `semantic_bi_graphdb_home` и `semantic_bi_graphdb_state`, порты localhost `7200` и `8081`. Если соответствующий ресурс уже существует, остановитесь и доложите оператору; не удаляйте и не переиспользуйте неизвестные данные.

Проверьте наличие архива и допустимый список его файлов, затем распакуйте его во временный приватный каталог:

```sh
umask 077
test -f /tmp/semantic-bi-vps-configs.tar.gz
tar -tzf /tmp/semantic-bi-vps-configs.tar.gz
install -d -m 700 /tmp/semantic-bi-vps-configs
tar --no-same-owner -xzf /tmp/semantic-bi-vps-configs.tar.gz -C /tmp/semantic-bi-vps-configs
```

Ожидаются ровно `graphdb.env`, `runtime.env`, `loader.yaml`, `monitor.env`, `graphdb.license` и `docker-config/config.json`. При неожиданном пути/файле остановитесь; не распаковывайте архив в checkout.

### 2.2 Установить файлы и права

Установите рабочие конфиги вне checkout. Loader и monitor читают конфиги как UID/GID `10001:10001`; GraphDB в образе 11.5.2 запускается от default root. Compose env и Docker config принадлежат пользователю развёртывания:

```sh
sudo install -d -o root -g root -m 711 /etc/semantic-bi
sudo install -d -o "$(id -u)" -g "$(id -gn)" -m 700 /etc/semantic-bi/docker-config
sudo install -o "$(id -u)" -g "$(id -gn)" -m 600 /tmp/semantic-bi-vps-configs/graphdb.env /etc/semantic-bi/graphdb.env
sudo install -o "$(id -u)" -g "$(id -gn)" -m 600 /tmp/semantic-bi-vps-configs/runtime.env /etc/semantic-bi/runtime.env
sudo install -o 10001 -g 10001 -m 600 /tmp/semantic-bi-vps-configs/loader.yaml /etc/semantic-bi/loader.yaml
sudo install -o 10001 -g 10001 -m 600 /tmp/semantic-bi-vps-configs/monitor.env /etc/semantic-bi/monitor.env
sudo install -o root -g root -m 600 /tmp/semantic-bi-vps-configs/graphdb.license /etc/semantic-bi/graphdb.license
sudo install -o "$(id -u)" -g "$(id -gn)" -m 600 /tmp/semantic-bi-vps-configs/docker-config/config.json /etc/semantic-bi/docker-config/config.json
```

Не выполняйте `cat`, `env`, `set -x` или команды, печатающие содержимое файлов. Проверяйте только имена и права (`stat`). Владелец рабочего runtime env должен иметь одинаковый image tag `v2.0.1` для обоих сервисов. GHCR login на VPS и ручной ввод PAT не нужны.

### 2.3 Проверить Compose и скачать образы

Все следующие команды запускаются из корня checkout:

```sh
docker compose -p semantic-bi-graphdb --env-file /etc/semantic-bi/graphdb.env -f deploy/graphdb/compose.vps.yaml config --quiet
DOCKER_CONFIG=/etc/semantic-bi/docker-config docker compose -p semantic-bi-runtime --env-file /etc/semantic-bi/runtime.env -f deploy/graphdb/compose.yaml config --quiet
docker compose -p semantic-bi-graphdb --env-file /etc/semantic-bi/graphdb.env -f deploy/graphdb/compose.vps.yaml pull graphdb
DOCKER_CONFIG=/etc/semantic-bi/docker-config docker compose -p semantic-bi-runtime --env-file /etc/semantic-bi/runtime.env -f deploy/graphdb/compose.yaml pull loader monitor
```

Если `pull` закрытого GHCR package отклонён, не просите пользователя вводить PAT через prompt и не выводите auth config. Остановитесь и сообщите, что нужен новый Docker auth config с `read:packages`.

Создайте отдельный пустой checkpoint volume и назначьте права loader:

```sh
docker volume create semantic_bi_graphdb_state
docker run --rm -v semantic_bi_graphdb_state:/state busybox:1.36 chown 10001:10001 /state
```

GraphDB data volume создастся при старте Compose; он должен отсутствовать до чистого запуска. Compose ограничивает GraphDB одним CPU и публикует порты только на localhost.

### 2.4 Создать GraphDB repository и проверить сеть

```sh
docker compose -p semantic-bi-graphdb --env-file /etc/semantic-bi/graphdb.env -f deploy/graphdb/compose.vps.yaml up -d graphdb
docker compose -p semantic-bi-graphdb --env-file /etc/semantic-bi/graphdb.env -f deploy/graphdb/compose.vps.yaml run --rm repository-init
docker compose -p semantic-bi-graphdb --env-file /etc/semantic-bi/graphdb.env -f deploy/graphdb/compose.vps.yaml run --rm repository-init
docker run --rm --network graphdb_default curlimages/curl:8.12.1 --fail --silent http://graphdb:7200/rest/repositories/bitrix24 >/dev/null
```

Оба запуска инициализатора должны завершиться с кодом `0`. Он создаёт repository, если его нет, и при каждом запуске подтверждает `owl2-rl` и `enableContextIndex=true`. Несовпадение приводит к ошибке без изменения существующего repository. Проверьте, что новый repository пустой и GraphDB license принята. Workbench доступен только через SSH-туннель `ssh -L 7200:127.0.0.1:7200 deploy@VPS`.

### 2.5 Выполнить первый strict цикл и остановиться на приёмке

До первого цикла убедитесь, что подготовленный S3 config указывает на контракт v2 и базовый snapshot доступен. Выполните ровно один цикл:

```sh
docker compose -p semantic-bi-runtime --env-file /etc/semantic-bi/runtime.env -f deploy/graphdb/compose.yaml run --rm loader --config /run/secrets/loader_config --once
```

Не запускайте фоновые loader/monitor до приёмки. Проверьте exit code `0`, логи без ошибок, обработанный snapshot и доступные инкременты, появление `/var/lib/semantic-bi/status.json`, соответствие bucket/portal/repository и RDF графов ожидаемому источнику. Если цикл неуспешен, не удаляйте repository или checkpoint: доложите причину и сохраняйте ту же пару ресурсов для разбора/повтора.

На этом этапе Codex CLI должен остановиться и предоставить оператору краткий отчёт по результатам первого цикла. Только после явного подтверждения приёмки переходите к постоянным сервисам.

### 2.6 После подтверждения приёмки

После подтверждения оператором:

```sh
docker compose -p semantic-bi-runtime --env-file /etc/semantic-bi/runtime.env -f deploy/graphdb/compose.yaml up -d loader monitor
docker compose -p semantic-bi-runtime --env-file /etc/semantic-bi/runtime.env -f deploy/graphdb/compose.yaml ps
```

Проверьте, что контейнеры работают без рестартов, monitor слушает `127.0.0.1:8081`, показывает тот же GraphDB repository и видит loader status. Для просмотра с рабочей машины используйте SSH tunnel `ssh -L 8081:127.0.0.1:8081 deploy@VPS`.

После установки удалите временные данные bundle, когда все файлы проверены:

```sh
rm -rf /tmp/semantic-bi-vps-configs /tmp/semantic-bi-vps-configs.tar.gz
```

Не удаляйте `/etc/semantic-bi`, Docker config или тома при обычной остановке/перезапуске. Не используйте `docker compose down -v`.
