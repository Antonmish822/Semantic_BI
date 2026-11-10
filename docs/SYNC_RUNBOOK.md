# Инструкция по запуску S3 → RDF загрузчика

Руководство для двух способов запуска текущего загрузчика `pipelines.bitrix24`: непосредственно в терминале и в Docker Compose. Оба способа читают события из S3 и записывают/удаляют именованные графы в RDF-хранилище. S3 загрузчик не изменяет.

## Перед запуском

Рабочий каталог — корень репозитория `Semantic_BI`:

```bash
cd "/путь/к/bitrix24_dashboard/Semantic_BI"
```

Нужны доступ к S3, работающий Fuseki или GraphDB с нужным dataset/repository и настроенные CCCR mappings. Для прямого локального запуска загрузчика поддерживается Linux `x86_64`, `uv` версии `0.9.10` и checkout `cccr_engine`. На Windows используйте Linux-контейнеры через Docker Desktop/WSL 2; на macOS — Docker Desktop. Текущий образ `linux/amd64` работает на Apple Silicon через эмуляцию, а нативный `linux/arm64` пока не поддержан из-за платформенного ограничения в корневом `pyproject.toml`. Если зависимости локального Linux окружения ещё не установлены:

```bash
uv sync --locked
```

Перед запуском выберите **один** способ и checkpoint, относящийся к нужному RDF-хранилищу. Не запускайте локальный и контейнерный процессы параллельно: даже если они используют копии одного checkpoint, оба могут одновременно менять одинаковые графы.

## Конфигурация

### Локальный запуск

Создайте отдельный локальный конфиг:

```bash
umask 077
cp deploy/loader/config.example.yaml deploy/loader/config.local.yaml
chmod 600 deploy/loader/config.local.yaml
```

В `deploy/loader/config.local.yaml` заполните доступы S3 и Fuseki. Для локального Fuseki укажите доступный с хоста URL dataset, например `http://localhost:3030/Persistent`; `/data` к нему добавлять не нужно. Настройте пути mappings относительно файла конфига:

```yaml
mapping:
  registry_path: ../../mappings/bitrix24/registry.json
  schemas_root: ../../mappings/bitrix24
checkpoint:
  path: /абсолютный/путь/к/постоянным-данным/semantic-bi/state.sqlite3
```

Относительные пути в YAML разрешаются относительно `deploy/loader`. Если нужно продолжить уже идущую синхронизацию, используйте актуальный checkpoint, соответствующий тому же dataset. Новый пустой checkpoint начинает загрузку с доступного полного снимка, но сам по себе не очищает старые графы в Fuseki.

### Docker Compose

В Compose используется `deploy/loader/config.yaml`. На новой установке скопируйте пример, заполните credentials и закройте доступ к файлу:

```bash
umask 077
cp deploy/loader/config.example.yaml deploy/loader/config.yaml
chmod 600 deploy/loader/config.yaml
```

Для контейнера задайте пути mappings `/app/mappings/bitrix24/registry.json` и `/app/mappings/bitrix24`. В `fuseki.url` укажите hostname и порт Fuseki, доступные из сети Docker, плюс dataset, например `http://jena-fuseki-owl2:3030/Persistent`. Имя сети в [`compose.yaml`](../deploy/loader/compose.yaml) должно совпадать с сетью контейнера Fuseki. Если Fuseki работает только в другой Docker-сети, подключите loader к этой сети; адрес `localhost` внутри loader указывает на сам loader.

Compose монтирует file-backed secret с host-правами, поэтому задайте UID/GID владельца файла и каталога checkpoint. На Linux:

```bash
export LOADER_UID="$(id -u)" LOADER_GID="$(id -g)"
```

В этом checkout уже настроены `deploy/loader/config.yaml`, сеть Fuseki и внешний volume `semantic_bi_loader_state_migrated`. В volume помещён checkpoint ранее работавшей синхронизации. Не удаляйте volume и не заменяйте checkpoint пустым файлом при продолжении этой загрузки.

Compose запускает опубликованные образы из закрытых GHCR packages. Войдите с PAT classic, у которого есть `read:packages` (токен вводится через stdin):

```bash
read -rsp 'GitHub token: ' GHCR_TOKEN; echo
printf '%s' "$GHCR_TOKEN" | docker login ghcr.io -u ВАШ_GITHUB_ЛОГИН --password-stdin
unset GHCR_TOKEN
```

Для production выберите версию релиза или точный SHA, после чего загрузите её:

```bash
export LOADER_IMAGE_TAG=v1.2.0
export MONITOR_IMAGE_TAG=v1.2.0
# Укажите существующий релизный тег, который был отправлен в Git.
docker compose -p semantic_bi -f deploy/loader/compose.yaml pull
```

Укажите существующий release tag (например `v1.2.0`) либо `sha-<полный-git-sha>`. Compose требует задать обе переменные; подвижные branch-теги не публикуются. Перед запуском или сменой версии выполняйте `pull`, поскольку `up` сам не обновляет локально имеющийся образ. На целевой машине сборка из исходников не выполняется. CI собирает образ, включая CCCR native engine, и выполняет проверки из Dockerfile.

## Выполнить один цикл

### В терминале

```bash
set -o pipefail
uv run python -m pipelines.bitrix24.cli \
  --config deploy/loader/config.local.yaml \
  --log-level INFO --once \
  2>&1 | tee -a /tmp/semantic-bi-sync.log
```

### В контейнере

Аргументы после имени сервиса `loader` заменяют команду по умолчанию Compose, поэтому передавайте путь к конфигу явно:

```bash
set -o pipefail
docker compose -p semantic_bi -f deploy/loader/compose.yaml run --rm loader \
  --config /run/secrets/loader_config --once \
  2>&1 | tee -a /tmp/semantic-bi-container-sync.log
```

Опция `--once` выполняет один цикл и завершает процесс. Цикл проверяет принятый snapshot и checkpoint, затем обрабатывает завершённые incremental runs до курсора экспортёра. При отставании checkpoint или изменении mappings проход может занять время. В начале цикла журнал может не обновляться, пока загрузчик читает состояние S3 и манифесты; затем появляются записи о применённых событиях и runs.

`set -o pipefail` сохраняет код завершения загрузчика при выводе через `tee`. Для просмотра файла журнала в другом терминале:

```bash
tail -f /tmp/semantic-bi-sync.log
```

Для контейнерного запуска подставьте `/tmp/semantic-bi-container-sync.log`.

## Запустить постоянную синхронизацию

### В терминале

Повторите локальную команду без `--once`:

```bash
set -o pipefail
uv run python -m pipelines.bitrix24.cli \
  --config deploy/loader/config.local.yaml \
  --log-level INFO \
  2>&1 | tee -a /tmp/semantic-bi-sync.log
```

Загрузчик начинает следующий цикл через `poll_interval_seconds` (по умолчанию 60 секунд). Просмотр остановите сочетанием `Ctrl+C`; процесс синхронизации завершится.

### В контейнере

Compose запускает CLI без `--once`, поэтому сервис циклически синхронизирует S3 и сам перезапускается после выхода:

```bash
export LOADER_UID="$(id -u)" LOADER_GID="$(id -g)"
docker compose -p semantic_bi -f deploy/loader/compose.yaml up -d
docker compose -p semantic_bi -f deploy/loader/compose.yaml logs -f loader
```

Для обновления только загрузчика сначала задайте новый `LOADER_IMAGE_TAG`, затем выполните `pull loader` и `up -d --no-deps loader`. Checkpoint сохранится во внешнем томе. Откат выполняется аналогично с предыдущим SHA-тегом.

`logs -f` только показывает журнал и следит за новыми строками. Нажатие `Ctrl+C` останавливает просмотр, контейнер остаётся работать в фоне.

Остановить контейнер, сохранив checkpoint:

```bash
docker compose -p semantic_bi -f deploy/loader/compose.yaml down
```

Не добавляйте `-v`: volume содержит checkpoint. Для продолжения используйте те же Compose project, конфиг, UID/GID и RDF dataset.

## Checkpoint и ошибки

- Не удаляйте и не обнуляйте checkpoint при обычном перезапуске. Новый пустой checkpoint не удаляет старые RDF-графы.
- Checkpoint привязан к backend и endpoint. Если URL меняется, сначала подтвердите, что новый URL указывает на тот же dataset; иначе загрузчик остановится с ошибкой несовпадения привязки. Не редактируйте SQLite вручную без резервной копии.
- При сетевом timeout записи в Fuseki текущая сущность не фиксируется в checkpoint и будет повторена. Повторная замена стабильного графа идемпотентна.
- Отсутствие строк в журнале сразу после запуска не обязательно означает зависание: начальная проверка S3 может выполняться до первой записи о применённом событии.
- Одна команда `--once` не гарантирует, что она поймает exporter runs, опубликованные после чтения `state.json`. Запустите следующий цикл или постоянный режим.
- Известные ограничения обработки manifests и восстановления описаны в [reliability backlog](SYNC_RELIABILITY_BACKLOG.md); полный алгоритм — в [описании загрузки](S3_TO_FUSEKI_LOADING.md).

## Дополнительные документы

- [Контракт интеграции](RDF_INTEGRATION.md)
- [README проекта](../README.md)
