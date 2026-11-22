# Развёртывание Semantic_BI на Linux VPS

## Краткий запуск

```sh
git clone '<URL-Semantic_BI>' /opt/Semantic_BI
cd /opt/Semantic_BI
git checkout '<deployment-tag-or-SHA-with-VPS-files>'
chmod +x deploy/vps.sh
deploy/vps.sh bootstrap --revision '<тот-же-deployment-tag-or-SHA>' --bundle /путь/к/готовому-bundle.tar.gz
deploy/vps.sh status
```

После приёмки первого импорта:

```sh
deploy/vps.sh start
```

Обновление до опубликованной версии:

```sh
deploy/vps.sh update --release '<опубликованный-тег-релиза>'
```

## Требования

Сценарий выполняет DevOps через SSH, Bash и Docker Compose. Codex CLI на сервере не нужен. Требуются Linux amd64, Bash 4+, Git, Python 3, `tar`, `curl`, `ss` (iproute2), `flock` (util-linux), `sha256sum` (coreutils), Docker Engine, Compose v2 и sudo/root для установки файлов в `/etc/semantic-bi`.

Скрипт проверяет минимум 2 CPU, 4 GiB памяти и 10 GiB свободного места на корневой файловой системе. Это только нижняя граница: диск должен вместить полный S3 snapshot и запас на рост RDF. Ориентир проекта — 4 vCPU, 8 GiB RAM и около 100 GB SSD. Порты 7200 и 8081 публикуются только на localhost VPS. GraphDB закреплён на 11.5.2; сценарий его не обновляет.

Если DevOps работает не от root, требуется безпарольный `sudo`. Перед первым запуском можно сверить среду:

```sh
uname -m
docker info --format '{{.ServerVersion}}'
docker compose version --short
df -h /
```

`--revision` обязателен: укажите tag или SHA deployment-версии, в которой уже есть `deploy/vps.sh` и это руководство. Скрипт проверяет, что revision разрешается в текущий `HEAD`, а checkout не содержит локальных изменений. Не используйте `v2.0.2` для checkout: этот опубликованный тег старше deployment-файлов и их не содержит.

## Bootstrap и первый импорт

`bootstrap` устанавливает рабочие конфиги в `/etc/semantic-bi`, проверяет Compose, скачивает образы, создаёт отдельные тома `semantic_bi_graphdb_home` и `semantic_bi_graphdb_state`, запускает GraphDB и `repository-init`, проверяет GraphDB из общей Docker-сети и запускает один detached-контейнер `semantic-bi-first-import` с `--once`. Постоянные loader и monitor на этом этапе не запускаются.

После запуска SSH-сессию можно закрыть. Для проверки результата и просмотра логов используйте:

```sh
deploy/vps.sh status
docker logs -f semantic-bi-first-import
```

Импорт принят, если контейнер завершился с кодом 0, а свежий `status.json` содержит `mode=once`, `state=idle`, `error=null`, `contract_version=2`. Команда `status` выводит ревизию, базовый manifest и применённый tip. Если контейнер уже остановлен, а файл показывает `running`, это остаточный статус после сбоя, а не текущий импорт.

При сбое сохраняйте GraphDB, checkpoint и контейнер для диагностики. После разбора DevOps может явно повторить импорт на той же паре томов:

```sh
deploy/vps.sh bootstrap --revision '<тот-же-тег-или-SHA>' --resume
```

`--resume` проверяет метки томов и неизменность конфигурации S3/GraphDB. Предыдущие логи неуспешной попытки сохраняются в `/var/lib/semantic-bi-deploy/previous-first-import.log`. Не удаляйте `semantic-bi-first-import` до сохранения кода выхода и диагностики. Не используйте `docker compose down -v`.

## Приёмка и постоянные сервисы

Вызов `start` означает приёмку первого импорта. Он запускает loader и monitor, проверяет локальный API монитора и ждёт свежий успешный цикл loader. Ожидание ограничено 30 минутами; при ошибке сервисы и тома сохраняются.

Проверить состояние, логи и status-файл можно так:

```sh
deploy/vps.sh status
docker compose -p semantic-bi-runtime --env-file /etc/semantic-bi/runtime.env -f deploy/graphdb/compose.yaml logs -f loader monitor
docker run --rm -v semantic_bi_graphdb_state:/state:ro python:3.13-alpine python -c 'import json; print(json.dumps(json.load(open("/state/status.json")), indent=2))'
```

Монитор доступен локально на `http://127.0.0.1:8081/api/status`. Для браузера используйте SSH-туннель: `ssh -L 8081:127.0.0.1:8081 deploy@VPS`.

## Обновление

`update` принимает явный опубликованный release tag, проверяет конфигурацию и режим `fast`, скачивает целевые образы до переключения и пересоздаёт только loader и monitor. GraphDB и тома не меняются. После пересоздания сценарий проверяет API монитора и ждёт свежий успешный цикл loader.

Предыдущая и целевая версии, а также последний этап записываются вне checkout в `/var/lib/semantic-bi-deploy`. При частичном сбое команда сообщает эти версии и рекомендует проверить фактически запущенные контейнеры через `deploy/vps.sh status`. Автоматического отката нет. Если для версии требуется обновить GraphDB, выполните отдельную процедуру.

## Проверки и ограничения

CI проверяет синтаксис сценария и валидирует оба Compose-файла на тестовых значениях без production secrets. Полный импорт с S3 проверяется DevOps на целевом VPS. Для отдельного приёмочного стенда используйте тестовый snapshot, отдельные Compose project names, сеть, тома и порты.

## Приложение. Готовый bundle

DevOps получает готовый закрытый архив по защищённому каналу и передаёт его команде `bootstrap`. Генерировать bundle на VPS не нужно. Не выводите содержимое архива, не помещайте архив или распакованные файлы в Git и удалите переданную копию после успешной установки.

Архив должен содержать ровно эти пути:

```text
graphdb.env
runtime.env
loader.yaml
monitor.env
graphdb.license
docker-config/
docker-config/config.json
```

Сценарий проверяет архив до установки: запрещает неожиданные пути, ссылки, дубликаты и специальные файлы; распаковывает его во временный приватный каталог и удаляет каталог при выходе. Конфигурация должна задавать одинаковые S3 bucket и portal ID для loader и monitor, repository `bitrix24`, адрес GraphDB `http://graphdb:7200`, путь статуса `/var/lib/semantic-bi/status.json` и `checkpoint.resume_mode: fast`. При `strict` скрипт завершится с указанием поля и не запустит loader. Теги loader и monitor должны совпадать. Переданный Docker auth config используется только для pull из GHCR.

Если bundle готовит доверенная рабочая машина, используется `scripts/prepare_vps_bundle.py`. Генератор читает локальные S3-настройки, шаблоны, лицензию и GHCR-only Docker auth config, а архив создаёт вне checkout с режимом `600`. Например:

```sh
python3 scripts/prepare_vps_bundle.py \
  --license-file /secure/path/graphdb.license \
  --docker-config /secure/docker-config/config.json \
  --image-tag '<опубликованный-тег>' \
  --output /tmp/semantic-bi-vps-configs.tar.gz
```

PAT с `read:packages` применяется только на доверенной машине при создании Docker auth config. Сам архив содержит S3 credentials, обратимо закодированные GHCR credentials и лицензию GraphDB, поэтому считается секретом.
