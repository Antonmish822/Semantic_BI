# Загрузчик v2 для GraphDB `bitrix24_v2_parallel`

Инструкция фиксирует локальный запуск загрузчика из образа с `cccr` и последующий переход на опубликованный образ GHCR. Команды выполняются из корня `Semantic_BI` на Linux.

## Текущее состояние

- Контейнер загрузчика: `semantic_bi_v2_parallel_loader`.
- Текущий образ: `ghcr.io/antonmish822/semantic-bi-loader:local`, локальный image ID `sha256:851f00a03174f440e6bf76956000ec1851605536abbe2a8c077514b0466814cd`.
- Локальный закрытый конфиг: `local-data/v2-parallel/container-loader.yaml` (права `0600`). Он указывает на bucket `garpix-bitrix24-v2-parallel-20261114`, portal `garpix-bitrix24-ru-v2-parallel` и GraphDB repository `bitrix24_v2_parallel`.
- Checkpoint и `status.json` контейнера: `local-data/v2-parallel/container-state/`. Это отдельная копия v2 checkpoint; исходный `local-data/v2-parallel/loader-state.sqlite3` сохранён.
- Контейнер использует `--network host`, потому что checkpoint привязан к GraphDB identity `http://127.0.0.1:7200/repositories/bitrix24_v2_parallel/rdf-graphs/service`. Не меняйте endpoint на Docker DNS alias без отдельной миграции/проверки привязки checkpoint.
- Конфиг задаёт `resume_mode: fast` и пустой `snapshot_reconcile_entity_types`. Fast mode может пропустить проверку байтов объекта, если ссылка точно совпадает с уже применённой записью checkpoint; новый объект и подтверждённые manifests всё равно проверяются загрузчиком. Snapshot этого exporter-а объявил пустой `complete_entity_types`, поэтому отсутствие сущностей не используется для удаления графов.
- Старый контейнер `semantic_bi_graphdb-loader-1` остановлен. Его Compose-конфиг относится к старому bucket/portal и `fuseki_migration_monitor`; его не запускайте для параллельной v2 загрузки.

На момент подготовки инструкции контейнер был `running`, `restart_count=0`; загрузчик находился в `waiting` без ошибки. Его checkpoint был целостен (`PRAGMA integrity_check=ok`), с epoch `local-20261114T122034Z-8c39a8de`, revision `7`, tip `20261114T154045Z-750804ae` и SHA tip manifest `1713bda292b09afbbc0e9c42a358de759f609c3bd17fa005fa98868cff124ab7`. Число активных сущностей checkpoint совпало с числом GraphDB графов по типам при read-only сверке.

## Запуск локального образа

Сборка включает native библиотеку `cccr` и встроенные проверки Dockerfile:

```bash
docker build -f deploy/loader/Dockerfile \
  -t ghcr.io/antonmish822/semantic-bi-loader:local .
```

После сборки `cccr` можно проверить без доступа к S3 или RDF:

```bash
docker run --rm --entrypoint python \
  ghcr.io/antonmish822/semantic-bi-loader:local \
  -c 'import cccr_py; print(cccr_py.__file__)'
```

Конфиг уже подготовлен отдельно от образа и репозитория. Убедитесь, что секреты не попали в Git, и запускайте непрерывный сервис:

```bash
chmod 600 local-data/v2-parallel/container-loader.yaml
mkdir -p local-data/v2-parallel/container-state
chmod 700 local-data/v2-parallel/container-state

docker run -d \
  --name semantic_bi_v2_parallel_loader \
  --restart unless-stopped \
  --network host \
  --user "$(id -u):$(id -g)" \
  --read-only \
  --tmpfs /tmp \
  --mount type=bind,src="$PWD/local-data/v2-parallel/container-loader.yaml",dst=/run/secrets/loader_config,readonly \
  --mount type=bind,src="$PWD/local-data/v2-parallel/container-state",dst=/var/lib/semantic-bi \
  ghcr.io/antonmish822/semantic-bi-loader:local \
  --config /run/secrets/loader_config --log-level WARNING
```

Для первого запуска из исходного локального checkpoint его копия создаётся через SQLite backup API после проверки, что источник не заблокирован активным процессом. Не копируйте работающий SQLite-файл вместе с WAL обычным `cp` и не подключайте исходный checkpoint параллельно к двум загрузчикам.

Проверяйте состояние без вывода событий или CRM payload:

```bash
docker inspect semantic_bi_v2_parallel_loader \
  --format 'state={{.State.Status}} restarts={{.RestartCount}} image={{.Config.Image}}'
cat local-data/v2-parallel/container-state/status.json
```

В status/checkpoint сверяйте `contract_version=2`, `epoch_id`, `base_manifest_sha256`, `applied_tip_run_id`, `applied_tip_manifest_sha256` и `observed_state_revision` с актуальным S3 `state.json`. Для SQLite используйте `PRAGMA integrity_check`; графы проверяйте read-only запросом к `bitrix24_v2_parallel` и сравнивайте типизированные counts с активными entity rows checkpoint. Успешный код завершения команды сам по себе не является проверкой результата.

Остановить или возобновить этот контейнер:

```bash
docker stop -t 30 semantic_bi_v2_parallel_loader
docker start semantic_bi_v2_parallel_loader
```

Не удаляйте каталог `local-data/v2-parallel/container-state`: он содержит позицию запущенного загрузчика.

## Переход на опубликованный образ GHCR

Образы публикуются GitHub Actions workflow-ом `.github/workflows/publish-images.yml` после push Git-тега `v*`. Используйте неизменяемый релизный тег `v<major>.<minor>.<patch>` либо `sha-<полный-git-sha>` из публикации. Не подменяйте образ подвижным branch-тегом.

1. Убедитесь, что нужный коммит прошёл CI и образ опубликован. Сохраните полный SHA коммита и выберите опубликованный тег.
2. Войдите в закрытый GHCR пакет с PAT, имеющим `read:packages`, затем загрузите именно версию loader:

   ```bash
   docker login ghcr.io
   export LOADER_IMAGE_TAG=vX.Y.Z  # либо sha-<полный-git-sha>
   export LOADER_IMAGE="ghcr.io/antonmish822/semantic-bi-loader:${LOADER_IMAGE_TAG}"
   docker pull --platform linux/amd64 "$LOADER_IMAGE"
   docker run --rm --platform linux/amd64 --entrypoint python "$LOADER_IMAGE" \
     -c 'import cccr_py; print(cccr_py.__file__)'
   ```

3. Остановите текущий контейнер и сделайте закрытую копию его конфигурации и checkpoint. Не редактируйте и не заменяйте текущие файлы:

   ```bash
   docker stop -t 30 semantic_bi_v2_parallel_loader
   umask 077
   BACKUP_DIR="local-data/v2-parallel/container-backups/$(date -u +%Y%m%dT%H%M%SZ)"
   mkdir -p "$BACKUP_DIR/state"
   chmod 700 "$BACKUP_DIR" "$BACKUP_DIR/state"
   cp -a local-data/v2-parallel/container-loader.yaml "$BACKUP_DIR/"
   cp -a local-data/v2-parallel/container-state/. "$BACKUP_DIR/state/"
   python - "$BACKUP_DIR/state/state.sqlite3" <<'PY'
   import sqlite3, sys
   db = sqlite3.connect(f"file:{sys.argv[1]}?mode=ro", uri=True)
   print(db.execute("PRAGMA integrity_check").fetchone()[0])
   db.close()
   PY
   ```

   Перед копированием дождитесь, чтобы контейнер перешёл в `exited`; это гарантирует закрытие SQLite. Убедитесь, что проверка вернула `ok`, а копия конфигурации и каталога checkpoint имеет права только владельца.

4. Оставьте остановленный local контейнер как точку возврата, сменив только его имя, и запустите опубликованный образ на том же конфиге и checkpoint:

   ```bash
   docker rename semantic_bi_v2_parallel_loader semantic_bi_v2_parallel_loader_local_backup
   docker run -d \
     --name semantic_bi_v2_parallel_loader \
     --restart unless-stopped \
     --platform linux/amd64 \
     --network host \
     --user "$(id -u):$(id -g)" \
     --read-only \
     --tmpfs /tmp \
     --mount type=bind,src="$PWD/local-data/v2-parallel/container-loader.yaml",dst=/run/secrets/loader_config,readonly \
     --mount type=bind,src="$PWD/local-data/v2-parallel/container-state",dst=/var/lib/semantic-bi \
     "$LOADER_IMAGE" \
     --config /run/secrets/loader_config --log-level WARNING
   ```

5. Проверьте, что новый контейнер не перезапускается, статус перешёл в `waiting`, protocol остаётся v2, а checkpoint продвигается по ожидаемому epoch/tip. Read-only сверьте counts GraphDB с активными entity rows checkpoint. Не удаляйте local backup контейнер и checkpoint backup, пока проверки не завершены.

### Откат

Если новая версия падает до продвижения checkpoint, остановите её, переименуйте контейнер в `semantic_bi_v2_parallel_loader_failed`, верните имя `semantic_bi_v2_parallel_loader_local_backup` и запустите его:

```bash
docker stop -t 30 semantic_bi_v2_parallel_loader
docker rename semantic_bi_v2_parallel_loader semantic_bi_v2_parallel_loader_failed
docker rename semantic_bi_v2_parallel_loader_local_backup semantic_bi_v2_parallel_loader
docker start semantic_bi_v2_parallel_loader
```

Если checkpoint или RDF уже продвинулись, перед откатом сверяйте согласованность пары. Для точного восстановления до состояния перед переходом восстанавливайте RDF repository и SQLite checkpoint вместе по процедуре [backup/restore](BACKUP_RESTORE_ARCHITECTURE.md); не подменяйте только один компонент пары. После восстановления запустите ранее проверенный image ID либо предыдущий SHA-тег.

## Монитор этого загрузчика

Локальный монитор работает в контейнере `semantic_bi_v2_parallel_monitor` из образа `ghcr.io/antonmish822/semantic-bi-monitor:local` и доступен на `http://127.0.0.1:8081`. Файл `rdf_store_monitor/.env.graphdb` указывает на v2 bucket/repository и `LOADER_STATUS_PATH=/var/lib/semantic-bi-v2-parallel/status.json`; каталог checkpoint загрузчика смонтирован монитору read-only в `/var/lib/semantic-bi-v2-parallel`. При пересоздании монитора сохраняйте этот mount и путь. Стандартный `deploy/graphdb/compose.yaml` по-прежнему монтирует прежний checkpoint volume и сам по себе не увидит status нового контейнера.
