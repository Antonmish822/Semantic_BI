# Контракт готовых контейнерных образов

Этот документ фиксирует имена образов и правила тегирования для развёртывания через GHCR без сборки на целевой машине. Workflow `.github/workflows/publish-images.yml` собирает и публикует образы, а `deploy/loader/compose.yaml` по умолчанию загружает тег текущей feature-ветки.

## Имена образов

Публикуются два независимых образа в namespace GitHub Container Registry владельца проекта:

| Компонент | Образ |
|---|---|
| S3 → RDF loader | `ghcr.io/antonmish822/semantic-bi-loader` |
| Fuseki monitor | `ghcr.io/antonmish822/semantic-bi-monitor` |

Образы публикуются как Linux/amd64: текущий Compose loader уже задаёт `platform: linux/amd64`, а CCCR native engine собирается внутри образа. Мультиархитектурную публикацию можно добавить отдельно после подтверждения поддержки и проверки native зависимости на других платформах.

## Теги

- `sha-<полный-git-sha>` — неизменяемый идентификатор каждой опубликованной сборки. Это основной тег для развёртывания и отката.
- `branch-<нормализованное-имя-ветки>` — удобный подвижный указатель на последнюю сборку ветки, например `branch-feat-entity-key-fuseki-sync`. Для закреплённого production-развёртывания не использовать.
- `v<major>.<minor>.<patch>` — неизменяемый тег релиза, например `v1.2.0`.

Подвижный `latest` не используется как версия развёртывания. Loader и monitor публикуются для одного commit и получают одинаковый SHA-тег, хотя остаются отдельными образами.

## Доступ и конфигурация

GHCR packages хранятся закрытыми. Workflow публикации использует `GITHUB_TOKEN` с `packages: write`; машине развёртывания нужен вход в GHCR с токеном, имеющим `read:packages`. Изменение видимости пакетов на public требует отдельного решения.

В образ включаются только код приложения, CCCR native dependency и runtime dependencies. S3/Fuseki credentials, `.env`, локальные YAML-конфиги и SQLite checkpoint остаются вне образов и передаются контейнерам при запуске.

## Публикация и развёртывание

1. Workflow получает закрытый `cccr_engine` через `SOURCECRAFT_SSH_KEY` и `SOURCECRAFT_KNOWN_HOSTS`, собирает оба образа и публикует их при push в `main`, `feat/entity-key-fuseki-sync` и при push релизного тега `v*`. Можно запустить workflow вручную через GitHub Actions.
2. `deploy/loader/compose.yaml` использует `LOADER_IMAGE_TAG` и `MONITOR_IMAGE_TAG`. Без переменных применяется подвижный тег `branch-feat-entity-key-fuseki-sync`; для рабочего закреплённого развёртывания задавайте `sha-<полный-git-sha>`.
3. Для скачивания закрытых пакетов войдите командой `docker login ghcr.io`, затем выполните `docker compose pull` и `docker compose up -d`. Перезапуск загрузчика сохраняет checkpoint в отдельном внешнем volume.

На целевом хосте можно выбрать фиксированную версию так:

```bash
# Замените SHA-пример полным SHA нужного коммита.
export LOADER_IMAGE_TAG=sha-0123456789abcdef0123456789abcdef01234567
export MONITOR_IMAGE_TAG=sha-0123456789abcdef0123456789abcdef01234567
docker compose -p semantic_bi -f deploy/loader/compose.yaml pull
docker compose -p semantic_bi -f deploy/loader/compose.yaml up -d
```

Для отката укажите предыдущий SHA-тег и повторите `pull`/`up -d`.
