# RAW‑слой: коннектор Bitrix24 → S3

Минимальный CLI для выгрузки сущностей Bitrix24 в S3 (RAW JSON) с постраничной загрузкой.

## Быстрый старт

1) Установить зависимости:

```
python -m venv .venv
.\.venv\Scripts\activate
pip install -r requirements.txt
```

2) Подготовить переменные окружения (см. `.env.example`).

3) Инициализировать бакет:

```
python scripts/init_bucket.py
```

4) Запустить выгрузку:

```
python -m bitrix_raw fetch-entity --entity deal --since 2024-01-01 --until 2024-01-31
```

## Переменные окружения

- `BITRIX_WEBHOOK_URL` — URL вебхука Bitrix24 (например, `https://example.bitrix24.ru/rest/1/xxxxxxxx/`).
- `S3_ENDPOINT_URL` — endpoint S3/MinIO (например, `http://localhost:9000`).
- `S3_ACCESS_KEY` — ключ доступа.
- `S3_SECRET_KEY` — секретный ключ.
- `S3_BUCKET` — имя бакета для RAW.
- `S3_PREFIX` — префикс хранения (по умолчанию `raw/bitrix24`).

Опционально:
- `S3_REGION` — регион (по умолчанию `us-east-1`).
- `S3_USE_SSL` — `true/false`.

## Поддерживаемые сущности

- `lead` → `crm.lead.list`
- `deal` → `crm.deal.list`
- `contact` → `crm.contact.list`
- `company` → `crm.company.list`
- `activity` → `crm.activity.list`
- `productrow` → `crm.item.productrow.list`

Список можно расширять в [`src/bitrix_raw/bitrix.py`](src/bitrix_raw/bitrix.py#L1).

## Формат хранения в S3

Ключи:

```
raw/bitrix24/<entity>/YYYY-MM-DD/batch_<N>.json
```

Каждый файл — массив объектов + метаданные выгрузки.

## Пример .env

См. файл [`.env.example`](.env.example#L1).
