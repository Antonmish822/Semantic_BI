# Онтология мини-дашборда

## Граф онтологии

Аксиома классификации активности хранится в отдельном именованном графе:

```text
urn:bitrix24:ontology:activity-classification
```

Файл с Turtle-источником: [`ontology/activity-classification.ttl`](ontology/activity-classification.ttl).

## Классификация Activity

Экземпляр будет выведен как `b24:Activity`, если в объединённом RDF-наборе у него есть оба литеральных признака:

- `b24:typeId` — тип активности Bitrix24;
- `b24:provider` — провайдер активности.

Аксиома намеренно задана как включение пересечения в класс `Activity`, а не как `owl:equivalentClass`. Она выводит класс по наличию признаков, не требуя обратного утверждения, что у каждого экземпляра `Activity` обязательно должны быть оба свойства.

В OWL 2 Functional Syntax выражение выглядит так:

```text
SubClassOf(
  ObjectIntersectionOf(
    DataSomeValuesFrom(b24:typeId rdfs:Literal)
    DataSomeValuesFrom(b24:provider rdfs:Literal)
  )
  b24:Activity
)
```

В RDF представлении OWL 2 ограничения на data properties сериализуются через `owl:onProperty` и `owl:someValuesFrom`; список классов в `owl:intersectionOf` представляет конъюнкцию условий ([OWL 2 Mapping to RDF Graphs](https://www.w3.org/TR/owl11-mapping-to-rdf/)).

## Проверка вывода

В конфигурации Fuseki проекта `Persistent` reasoner строит вычисляемый default graph из объединения именованных графов. Аксиома лежит в отдельном графе, а данные активности — в графах синхронизатора. Проверять выведенные типы следует в default graph, без `GRAPH <...>`:

```sparql
PREFIX b24: <https://example.org/bitrix24#>
PREFIX rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>

SELECT DISTINCT ?activity
WHERE {
  ?activity rdf:type b24:Activity
}
LIMIT 20
```

Такая проверка читает выведенное представление. Аксиома не добавляет `rdf:type` в исходные именованные графы сущностей.

Аксиома загружена в указанный граф Fuseki из `activity-classification.ttl`. Fuseki вернул HTTP 201, чтение графа подтвердило 20 триплетов. Проверочный `ASK` для существующей активности вернул `true`.

## Граница классификации

Это простое правило опирается на схему CCCR `activity.json`, где `typeId` и `provider` являются литеральными свойствами активности. Если позднее другие типы сущностей начнут использовать эту же пару свойств, они тоже будут классифицированы как `Activity`; тогда условие нужно будет уточнить по данным и бизнес-правилам.
