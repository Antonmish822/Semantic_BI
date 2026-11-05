#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <chrono>
#include <cstdio>
#include <format>
#include <memory_resource>
#include <string_view>
#include <variant>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<registry>
    <row id="1">
        <cell label="Год введения ООП">2009</cell>
        <cell label="Уровень образования">СПО</cell>
        <cell label="Профиль">универсальный</cell>
        <cell label="Дата утверждения ООП">2018-10-28</cell>
        <cell label="Номер приказа об утверждении ООП">3-У</cell>
        <cell label="Дата принятия на педагогическом совете">2018-12-12</cell>
        <cell label="Номер протокола педагогического совета">7</cell>
    </row>
</registry>
)";

constexpr std::string_view kSchema = R"JSON({
  "description": {
    "context":{
      "subject":{
        "compute":{
          "xquery":"./registry/row"
        }
      }
    },
    "about": {
      "compute": {
        "xquery": "concat('https://lipers24.ru/', .//cell[@label='Уровень образования']/text(), '_', .//cell[@label='Год введения ООП']/text(), '_', .//cell[@label='Профиль']/text())"
      }
    },
    "properties": [
      {
        "QName": { "NameSpace": "http://w3.org", "PrefixedName": "rdf:type" },
        "URI": "https://lipers24.ru"
      },
      {
        "QName": { "NameSpace": "http://w3.org", "PrefixedName": "rdfs:label" },
        "literal": {
          "compute": {
            "xquery": "concat('Образовательная программа ', .//cell[@label='Уровень образования']/text(), ' (', .//cell[@label='Год введения ООП']/text(), ', ', .//cell[@label='Профиль']/text(), ')')"
          }
        }
      },
      {
        "QName": { "NameSpace": "https://lipers24.ru", "PrefixedName": "lipers24:годВведения" },
        "literal": { "compute": { "xquery": ".//cell[@label='Год введения ООП']/text()" } },
        "datatype": "http://w3.org#gYear"
      },
      {
        "QName": { "NameSpace": "https://lipers24.ru", "PrefixedName": "lipers24:профиль" },
        "literal": { "compute": { "xquery": ".//cell[@label='Профиль']/text()" } }
      },
      {
        "context": {
          "local_level": { "compute": { "xquery": ".//cell[@label='Уровень образования']/text()" } }
        },
        "properties": [
          {
            "QName": { "NameSpace": "https://lipers24.ru", "PrefixedName": "lipers24:уровеньОбразования" },
            "URI": {
              "compute": {
                "sparql": "PREFIX edu: <https://lipers24.ru> SELECT ?levelUri WHERE { ?levelUri rdf:type edu:EducationLevel ; edu:shortName {{local_level}} }"
              }
            }
          }
        ]
      },
      {
        "QName": { "NameSpace": "https://lipers24.ru", "PrefixedName": "lipers24:датаУтверждения" },
        "literal": { "compute": { "xquery": ".//cell[@label='Дата утверждения ООП']/text()" } },
        "datatype": "http://w3.org#date"
      },
      {
        "QName": { "NameSpace": "https://lipers24.ru", "PrefixedName": "lipers24:номерПриказа" },
        "literal": { "compute": { "xquery": ".//cell[@label='Номер приказа об утверждении ООП']/text()" } }
      },
      {
        "QName": { "NameSpace": "https://lipers24.ru", "PrefixedName": "lipers24:датаПедсовета" },
        "literal": { "compute": { "xquery": ".//cell[@label='Дата принятия на педагогическом совете']/text()" } },
        "datatype": "http://w3.org#date"
      },
      {
        "QName": { "NameSpace": "https://lipers24.ru", "PrefixedName": "lipers24:номерПротокола" },
        "literal": { "compute": { "xquery": ".//cell[@label='Номер протокола педагогического совета']/text()" } },
        "datatype": "http://w3.org#integer"
      }
    ]
  }
})JSON";

constexpr std::string_view kSeed = R"(
<https://lipers24.ru/SPO> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <https://lipers24.ru#EducationLevel> .
<https://lipers24.ru/SPO> <https://lipers24.ru#shortName> "СПО" .
)";

[[nodiscard]] bool HasIriObject(const cccr::rdf::Graph& graph,
                                std::string_view subject,
                                std::string_view predicate,
                                std::string_view object) {
  for (const auto& triple : graph.Triples()) {
    if (triple.subject.value == subject &&
        triple.predicate.value == predicate &&
        cccr::rdf::TermIsIri(triple.object, object)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool HasLiteralObject(const cccr::rdf::Graph& graph,
                                    std::string_view subject,
                                    std::string_view predicate,
                                    std::string_view object) {
  for (const auto& triple : graph.Triples()) {
    if (triple.subject.value == subject &&
        triple.predicate.value == predicate &&
        cccr::rdf::TermIsLiteral(triple.object, object)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool HasTypedLiteral(const cccr::rdf::Graph& graph,
                                   std::string_view subject,
                                   std::string_view predicate,
                                   std::string_view object,
                                   std::string_view datatype) {
  for (const auto& triple : graph.Triples()) {
    if (triple.subject.value != subject ||
        triple.predicate.value != predicate) {
      continue;
    }
    const auto* lit = std::get_if<cccr::rdf::Literal>(&triple.object);
    if (lit != nullptr && lit->value == object && lit->datatype &&
        *lit->datatype == datatype) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST_SUITE("cccr::engine::Run") {
  TEST_CASE("cccr::engine::Run example smoke") {
    SUBCASE("пример.md end-to-end key triples") {
      std::pmr::monotonic_buffer_resource pool(1024 * 1024);

      const auto start = std::chrono::high_resolution_clock::now();

      const auto schema =
          cccr::json::LoadSchema(kSchema, {.ctx = {.allocator = &pool}});
      // REQUIRE(schema.has_value());

      const auto knowledge =
          cccr::rdf::LoadNTriples(kSeed, {.allocator = &pool});
      // REQUIRE(knowledge.has_value());

      cccr::xml::XmlSource source{{.allocator = &pool}};
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(
          *schema, source,
          {.knowledge = *knowledge, .ctx = {.allocator = &pool}});
      // REQUIRE(graph.has_value());

      const auto end = std::chrono::high_resolution_clock::now();
      const auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();
      std::fprintf(stdout, "%s\n",
                   std::format("Execution time: {} ms", duration).c_str());
      {
        const auto dump = cccr::rdf::ToRdfXml(*knowledge);
        std::fwrite(dump.data(), 1, dump.size(), stdout);
        std::fputc('\n', stdout);
      }

      constexpr std::string_view kSubject =
          "https://lipers24.ru/СПО_2009_универсальный";

      CHECK_EQ(graph->Size(), 9);
      CHECK_EQ(HasIriObject(*graph, kSubject, "http://w3.org#type",
                            "https://lipers24.ru"),
               true);
      CHECK_EQ(HasLiteralObject(
                   *graph, kSubject, "http://w3.org#label",
                   "Образовательная программа СПО (2009, универсальный)"),
               true);
      CHECK_EQ(HasLiteralObject(*graph, kSubject,
                                "https://lipers24.ru#годВведения", "2009"),
               true);
      CHECK_EQ(
          HasTypedLiteral(*graph, kSubject, "https://lipers24.ru#годВведения",
                          "2009", "http://w3.org#gYear"),
          true);
      CHECK_EQ(HasLiteralObject(*graph, kSubject, "https://lipers24.ru#профиль",
                                "универсальный"),
               true);
      CHECK_EQ(HasIriObject(*graph, kSubject,
                            "https://lipers24.ru#уровеньОбразования",
                            "https://lipers24.ru/SPO"),
               true);
      CHECK_EQ(
          HasLiteralObject(*graph, kSubject,
                           "https://lipers24.ru#датаУтверждения", "2018-10-28"),
          true);
      CHECK_EQ(HasTypedLiteral(*graph, kSubject,
                               "https://lipers24.ru#датаУтверждения",
                               "2018-10-28", "http://w3.org#date"),
               true);
      CHECK_EQ(HasLiteralObject(*graph, kSubject,
                                "https://lipers24.ru#номерПриказа", "3-У"),
               true);
      CHECK_EQ(
          HasLiteralObject(*graph, kSubject,
                           "https://lipers24.ru#датаПедсовета", "2018-12-12"),
          true);
      CHECK_EQ(
          HasTypedLiteral(*graph, kSubject, "https://lipers24.ru#датаПедсовета",
                          "2018-12-12", "http://w3.org#date"),
          true);
      CHECK_EQ(HasLiteralObject(*graph, kSubject,
                                "https://lipers24.ru#номерПротокола", "7"),
               true);
      CHECK_EQ(HasTypedLiteral(*graph, kSubject,
                               "https://lipers24.ru#номерПротокола", "7",
                               "http://w3.org#integer"),
               true);

      const auto rdfxml = cccr::rdf::ToRdfXml(*graph);
      CHECK_EQ(rdfxml.contains("rdf:datatype=\"http://w3.org#gYear\""), true);
      CHECK_EQ(rdfxml.contains("rdf:datatype=\"http://w3.org#date\""), true);
      CHECK_EQ(rdfxml.contains("rdf:datatype=\"http://w3.org#integer\""), true);
    }
  }
}
