# Загрузчик S3 → RDF: контракт 2

## Выбор протокола и гарантии

`pipelines.bitrix24.cli` читает один сырой `state.json`. Путь выбирается только
после проверки `contract_version`: legacy state без поля относится к v1;
значение `2` проходит строгую проверку структуры и канонических байтов. Любое
другое значение или повреждённый v2 указатель останавливает цикл. Checkpoint с
`contract_version: 2` нельзя запустить как v1, а checkpoint v1 нельзя
автоматически принять как v2. Для same-store перехода подготовьте отдельный
checkpoint из остановленной копии v1: entity registry сохраняется, старый run
cursor сбрасывается. Для нового пустого RDF dataset используйте отдельный пустой
SQLite файл.

V2 проверяет сырые канонические байты state, base и incremental manifests,
проверяет SHA тела каждого события, независимо вычисляет event ID алгоритма 3 и
сверяет ключ, entity key, graph ID, epoch и counts. Очередь строится от tip к
null или точному checkpoint ancestor и применяется от старого к новому. Backfill
вне цепочки не применяется. Snapshot обрабатывается до incremental; reconciliation
затрагивает только пересечение `complete_entity_types` и локальной настройки
`snapshot_reconcile_entity_types`. Activity не удаляется по отсутствию.

Сверка реализации экспортёра с нормативным proposal и четырьмя опубликованными
контрольными векторами не выявила расхождений в canonical JSON, timestamp
normalization, composite identity и event ID algorithm 3. State требует поля
`contract_version`, `revision`, `epoch_id`, `base_manifest_key`, base SHA и
парные tip ID/SHA; неизвестные дополнительные state поля игнорируются. Манифест
требует version/run/mode/epoch, ссылку на предыдущий run с SHA, complete status,
ровно шесть counts и уникальные object refs; snapshot также требует completeness
поля и временные границы, incremental/backfill — positive window. Дополнительные
поля манифеста вроде `created_at` и `dictionary_snapshot_id` допускаются. Event
envelope сверяется по полям сериализатора exporter; событие должно быть схемы 4,
алгоритма 3, `upsert` и иметь canonical JSON тело. Ни один runtime-модуль
экспортёра не импортируется.

Checkpoint сущности записывается после успешной RDF операции. Сбой после RDF до
SQLite приводит к идемпотентному повтору. Run фиксируется после всех событий,
snapshot — после всех событий и reconciliation. Если объект повреждён или
недоступен, вершина не продвигается. Диагностика не содержит CRM payload.

`checkpoint.resume_mode: strict` повторно проверяет тела snapshot и события
checkpoint boundary run, а также проверяет каждое событие новых звеньев цепочки.
Он не обходит снова историю старше уже подтверждённого checkpoint ancestor.
Для полного исторического аудита используйте новый изолированный checkpoint и
загрузку от snapshot в strict режиме. `fast` пропускает тело только при
совпадении S3 key, SHA ссылки, event ID, graph URI, mapping fingerprint и entity
checkpoint; последующая порча пропущенного объекта не обнаруживается. Перед
приёмкой и выпуском используйте strict, fast не является аудитом S3.

Лимиты задаются в `contract_v2_limits`: `max_chain_length`,
`max_objects_per_run`, `max_objects_per_cycle`, `max_state_bytes`,
`max_manifest_bytes`, `max_event_bytes`, `max_cycle_read_seconds` и
`s3_read_timeout_seconds`.
Превышение любого лимита блокирует цикл. Пример конфигурации содержит значения
по умолчанию.

## Status и backup

`status.json` включает версию контракта, epoch, base SHA, применённые run/SHA и
наблюдаемую state revision. Используйте поля `applied_tip_run_id` и
`applied_tip_manifest_sha256`, а не v1 `last_run_id`. Статусы `waiting`,
`blocked`, `error` не содержат payload или credentials.

Существующий backup barrier завершает цикл, сбрасывает WAL и закрывает SQLite;
затем backup сохраняет RDF dataset/repository и checkpoint. Backup manifest
фиксирует epoch, base SHA, вершину и revision, а verify/restore сверяют эти
поля с SQLite. Перед восстановлением проверьте пакет `backup verify`, backend и
endpoint identity. Восстанавливайте RDF и SQLite как одну пару из одного
backup-пакета; не редактируйте SHA вручную и не восстанавливайте один компонент.

## Подготовленная процедура выпуска

Выполнять на рабочем RDF/S3 только после успешной изолированной приёмки этапа 7
и в согласованное окно.

1. Зафиксировать commit и tag образа, backend/dataset, portal, mapping fingerprint,
   ожидаемые `epoch_id`, `base_manifest_sha256` и `tip_run_id`/SHA. Read-only
   проверкой убедиться, что state и нужные объекты доступны и проходят strict.
2. Остановить старый загрузчик и экспортёр по их runbook. Убедиться, что никто
   не пишет в RDF или checkpoint во время backup.
3. Создать backup пары RDF + SQLite через barrier по
   [архитектуре backup](BACKUP_RESTORE_ARCHITECTURE.md), затем выполнить
   `backup verify`. Сохранить пакет и SHA256SUMS вне контейнера.
4. Подготовить v2 конфигурацию с `resume_mode: strict`, верным backend identity,
   локально разрешёнными типами reconciliation и отдельным checkpoint путём.
   Сначала запустить образ на изолированных S3 и RDF ресурсах.
5. На остановленной копии v1 checkpoint подготовить отдельный v2 файл. Команда
   блокирует активного писателя, проверяет SQLite и backend identity, сохраняет
   entity rows, сбрасывает legacy run cursor и не меняет исходный файл:

   ```sh
   python -m pipelines.bitrix24.prepare_v2_checkpoint \
     /backup/v1-checkpoint.sqlite3 /var/lib/semantic-bi/state-v2.sqlite3 \
     --backend fuseki --identity 'http://fuseki.example/ds/data'
   ```

   Настроить загрузчик на `state-v2.sqlite3` и применить новую полную эпоху. Не
   переносить `last_run_id` v1 как tip. После публикации `contract_version: 2`
   не запускать v1 образ.
6. Сверить число и список графов по entity type/key, контрольные RDF значения,
   epoch/base SHA и применённую вершину с state. Дождаться, чтобы каждый активный
   loader достиг подтверждённого tip (или документированного более нового tip).
7. Включить обычный опрос и экспортёр после сверки. Сохранить старый checkpoint
   и backup на согласованный срок retention для отстающих клиентов.
8. При ошибке остановить загрузчик и сохранить безопасный status/log. Проверить
   state/manifests read-only. Восстановить backup RDF + SQLite как пару либо
   исправить источник/код и повторить цикл. SHA в checkpoint вручную не менять.

Критерий завершения каждого loader: `status.json.contract_version == 2`,
ожидаемые epoch/base SHA, `applied_tip_run_id` и SHA совпадают с подтверждённой
вершиной, сверка графов проходит, а backup verify подтверждает восстановимость
пары.
