# Краткая памятка ИИ-агенту: GraphDB API

## Подключение

- Уточни адрес сервера и точный ID repository. В текущем окружении: `http://localhost:7200`, `fuseki_migration_monitor`.
- SPARQL endpoint repository: `http://<host>:7200/repositories/<repository-id>`.
- Для контейнера в сети GraphDB используй DNS-имя `http://graphdb:7200/repositories/<repository-id>`.
- Если включена авторизация, передавай учётные данные через Basic Auth из защищённой конфигурации. Не выводи и не записывай секреты в логи, ответы или команды с видимыми аргументами.

## SPARQL-запросы

Отправляй запросы методом `POST` с `Content-Type: application/x-www-form-urlencoded` и полем `query`. Для результатов запросов задавай `Accept: application/sparql-results+json`.

Перед изменением данных проверь repository read-only запросом. Для именованных графов используй явный шаблон тройки — GraphDB может не вернуть графы для пустого шаблона `GRAPH ?g {}`:

```sparql
SELECT DISTINCT ?g WHERE {
  GRAPH ?g { ?s ?p ?o }
}
```

Ограничивай запрос нужным repository и namespace графов. Перед массовой выборкой оцени объём; для выгрузки всех графов сохраняй их IRI в формате N-Quads или TriG.

## Запись и удаление

- SPARQL Update отправляется как `POST` с параметром `update` на endpoint repository.
- Для загрузки RDF в repository используется `POST /repositories/<repository-id>/statements`; формат указывается через `Content-Type`.
- Для операций над отдельным именованным графом загрузчик проекта использует Graph Store Protocol endpoint `/repositories/<repository-id>/rdf-graphs/service` с параметром `graph=<IRI>`: `PUT` заменяет граф, `DELETE` удаляет его.
- Перед записью проверь точный repository и IRI графа. Для миграции загружай N-Quads только в пустой целевой repository и сверяй список графов и число триплетов после импорта.
- Не выполняй `CLEAR`, `DROP`, `DELETE` или изменение конфигурации repository, если задача явно не требует этой операции.

## REST API и ошибки

- REST API управления repository находится под `/rest/repositories`.
- Перед изменением конфигурации прочитай текущие параметры; обновляй только необходимые значения. Изменённая настройка может требовать перезапуска repository.
- На `401/403` проверь способ авторизации и права; на `404` — URL и ID repository; на `5xx` — состояние GraphDB и его логи. Не повторяй POST-запись вслепую после сетевого таймаута: сначала проверь, применились ли данные.
- Различай ошибки соединения и пустые результаты: пустая таблица SPARQL не доказывает, что repository недоступен.
