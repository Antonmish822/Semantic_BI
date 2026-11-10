# Контракт готовых контейнерных образов

Этот документ фиксирует имена образов и правила тегирования для развёртывания через GHCR без сборки на целевой машине. Workflow `.github/workflows/publish-images.yml` собирает и публикует образы, а `deploy/loader/compose.yaml` по умолчанию загружает тег текущей feature-ветки.

## Имена образов

Публикуются два независимых образа в namespace GitHub Container Registry владельца проекта:

| Компонент | Образ |
|---|---|
| S3 → RDF loader | `ghcr.io/antonmish822/semantic-bi-loader` |
| RDF store monitor | `ghcr.io/antonmish822/semantic-bi-monitor` |

Оба образа публикуются как `linux/amd64`; Compose закрепляет эту архитектуру. На Windows используйте Docker Desktop в режиме Linux containers с backend WSL 2. На macOS требуется Docker Desktop: Intel Mac запускает образ нативно, а Apple Silicon — через эмуляцию amd64, что может быть медленнее. Docker Desktop описывает [режим WSL 2](https://docs.docker.com/desktop/features/wsl/) и [эмуляцию платформ](https://docs.docker.com/build/building/multi-platform/).

Нативный `linux/arm64` образ пока не заявлен: корневой `pyproject.toml` ограничивает окружение Linux `x86_64`, хотя некоторые зависимости CCCR имеют arm64-ассеты. Для нативного запуска на Apple Silicon потребуется отдельная проверка и изменение матрицы платформ, сборки `cccr_engine` и CI. Текущий amd64-образ можно использовать через эмуляцию.

## Теги

- `v<major>.<minor>.<patch>` — неизменяемый тег релиза, например `v1.2.0`; это рекомендуемый тег развёртывания.
- `sha-<полный-git-sha>` — неизменяемый тег конкретного коммита, публикуемый вместе с релизом для точного отката.

Подвижные `latest` и branch-теги не публикуются. Loader и monitor публикуются только после push Git-тега `v*` и получают одинаковые version и SHA-теги, хотя остаются отдельными образами.

## Доступ и конфигурация

GHCR packages хранятся закрытыми. Workflow публикации использует `GITHUB_TOKEN` с `packages: write`; машине развёртывания нужен вход в GHCR с токеном, имеющим `read:packages`. Изменение видимости пакетов на public требует отдельного решения.

В образ включаются только код приложения, CCCR native dependency и runtime dependencies. S3/Fuseki credentials, `.env`, локальные YAML-конфиги и SQLite checkpoint остаются вне образов и передаются контейнерам при запуске.

## Публикация и развёртывание

1. Workflow получает закрытый `cccr_engine` через `SOURCECRAFT_SSH_KEY` и `SOURCECRAFT_KNOWN_HOSTS`, собирает оба образа и публикует их только при push Git-тега `v*`. Push в ветки может запустить CI-проверки, но не публикует образы.
2. `deploy/loader/compose.yaml` требует явные `LOADER_IMAGE_TAG` и `MONITOR_IMAGE_TAG`. Укажите один и тот же release tag (`v1.2.0`) либо SHA tag (`sha-<полный-git-sha>`); Compose не подставляет устаревающий branch tag.
3. Для скачивания закрытых пакетов войдите командой `docker login ghcr.io`, затем выполните `docker compose pull` и `docker compose up -d`. Перезапуск загрузчика сохраняет checkpoint в отдельном внешнем volume.

На целевом хосте можно выбрать фиксированную версию так:

```bash
export LOADER_IMAGE_TAG=v1.2.0
export MONITOR_IMAGE_TAG=v1.2.0
# Укажите существующий релизный тег, который был отправлен в Git.
docker compose -p semantic_bi -f deploy/loader/compose.yaml pull
docker compose -p semantic_bi -f deploy/loader/compose.yaml up -d
```

Для отката укажите предыдущий SHA-тег и повторите `pull`/`up -d`.
