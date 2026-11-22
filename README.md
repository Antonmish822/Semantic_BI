# Semantic_BI

Semantic_BI загружает данные Bitrix24 из S3 в RDF-хранилище и предоставляет их в виде именованных графов для семантической аналитики. Проект преобразует JSON-события экспортёра через CCCR mappings и поддерживает два backend: Apache Jena Fuseki и GraphDB.

Главные компоненты — v2 S3→RDF-загрузчик в `pipelines/bitrix24` и `rdf_store_monitor`, интерфейс чтения данных из выбранного RDF-хранилища (Fuseki или GraphDB).

Требования среды и инструкции запуска загрузчика приведены в [runbook S3 → RDF](docs/SYNC_RUNBOOK.md).

Для оперативного контроля активных сделок, бездействия по сделкам и нагрузки сотрудников есть локальный [CRM Pulse Monitor](../crm_pulse_monitor/README.md) в корне проекта. Он читает repository `bitrix24_v2_parallel` и не изменяет RDF-хранилище.

Архивные инструкции по переносу графов из Fuseki в GraphDB: [миграция](docs/archive/GRAPHDB_PARALLEL_MIGRATION.md) и [настройка связки](docs/archive/GRAPHDB_PARALLEL_SETUP_INSTRUCTIONS.md).


Краткие правила работы ИИ-агента с GraphDB API собраны в [памятке GraphDB API](docs/GRAPHDB_AGENT_API_GUIDE.md).

Согласованный backup/restore RDF-хранилища и v2 loader checkpoint для GraphDB и Fuseki описан в [архитектуре и инструкции резервного копирования](docs/BACKUP_RESTORE_ARCHITECTURE.md). Она включает онлайн pause barrier и offline-режим. Для Fuseki используйте [deploy/fuseki/compose.yaml](deploy/fuseki/compose.yaml) и локальную конфигурацию на основе [config.example.yaml](deploy/fuseki/config.example.yaml).

Загрузчик поддерживает отдельный consumer путь [контракта S3 v2](docs/LOADER_CONTRACT_V2_RUNBOOK.md); порядок строгой проверки, подготовки checkpoint, backup и выпуска описан в runbook.

## Что делает загрузчик

Для каждой CRM-сущности загрузчик вычисляет стабильный URI именованного графа по `entity_key`. Новая версия сущности полностью заменяет её граф; событие удаления удаляет граф. SQLite checkpoint хранит принятый snapshot, позиции обработки и применённые версии mappings. Checkpoint должен сохраняться между запусками и использоваться только с тем RDF-хранилищем, которому он принадлежит.

### Разовый цикл

Запуск с `--once` выполняет один цикл и завершает процесс. При пустом checkpoint загрузчик применяет полный snapshot v2, сверяет сущности только для настроенных полных типов и применяет доступную цепочку incremental runs до объявленного tip. Это режим одного цикла, а не отдельный режим «только первичная загрузка».

### Инкрементальная обработка

При последующих циклах загрузчик проверяет v2 state и проходит цепочку манифестов по ссылкам и SHA до своего checkpoint. `status.json` служит диагностикой, а не подтверждением запуска. Загрузчик проверяет SHA-256 и идентичность событий; `upsert` заменяет граф через Graph Store `PUT`, `delete` удаляет граф. Позиции сохраняются в SQLite checkpoint. После перезапуска безопасный повтор продолжает обработку с checkpoint.

Без `--once` загрузчик выполняет цикл регулярно с интервалом `poll_interval_seconds`. Подробности контракта и восстановления описаны в [RDF_INTEGRATION.md](docs/RDF_INTEGRATION.md) и [runbook контракта v2](docs/LOADER_CONTRACT_V2_RUNBOOK.md); известные ограничения собраны в [reliability backlog](docs/SYNC_RELIABILITY_BACKLOG.md).

## Развёртывание

Чистое развёртывание GraphDB, загрузчика S3 → RDF и монитора на одном Linux VPS описано в [runbook для DevOps](deploy/graphdb/VPS.md). Запускайте `deploy/vps.sh bootstrap --revision <tag-or-SHA> --bundle /secure/path/bundle.tar.gz`, проверяйте первый detached-импорт командой `deploy/vps.sh status`, затем принимайте его и запускайте постоянные сервисы через `deploy/vps.sh start`. Скрипт проверяет, что revision checkout совпадает с указанным tag/SHA, устанавливает закрытые конфиги, создаёт пустые тома и repository, выполняет `--once` в режиме `fast` и сохраняет код завершения для проверки после переподключения SSH. В `fast` загрузчик не перечитывает хеши уже применённых объектов при догонке последующих инкрементов. На доверенной рабочей машине bundle готовит `python3 scripts/prepare_vps_bundle.py`; в него входят секреты S3, Docker auth config для GHCR и лицензия. Передавайте bundle по SSH отдельно от Git checkout и не переносите локальные RDF-данные или checkpoint.

Имена GHCR-образов и правила закрепления версий описаны в [контракте контейнерных образов](docs/CONTAINER_IMAGE_CONTRACT.md). GitHub Actions использует `GITHUB_TOKEN` с `packages: write` для публикации. Для скачивания приватных образов оператор заранее создаёт Docker auth config на доверенной машине с PAT classic `read:packages`; генератор включает его в закрытый bundle. Docker CLI на VPS получает этот config через защищённую передачу, PAT в терминал VPS вводить не нужно.

Для локальной разработки и ручного цикла используйте [runbook S3 → RDF](docs/SYNC_RUNBOOK.md). Для GraphDB задайте `backend: graphdb`, `graphdb.url`, repository и отдельный checkpoint; не добавляйте конфигурацию с credentials или SQLite в Git.

Для настройки GraphDB и правил миграции см. [план рефакторинга](docs/plans/ARCHITECTURE_REFACTOR_PLAN.md) и [требования к проверке](docs/plans/testing_requirements.md).
