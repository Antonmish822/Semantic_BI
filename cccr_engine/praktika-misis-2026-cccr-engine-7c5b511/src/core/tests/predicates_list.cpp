#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <string_view>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<item><value>42</value></item>
)";

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

}  // namespace

TEST_SUITE("cccr::engine::Run PredicatesList") {
  TEST_CASE("cccr::engine::Run predicates list") {
    SUBCASE("one literal applied to each listed predicate") {
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [
      {
        "context": {
          "predicates": [
            "https://example.org#code",
            "https://example.org#altCode"
          ]
        },
        "literal": { "compute": { "xquery": "/item/value/text()" } }
      }
    ]
  }
})JSON";

      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE(graph.has_value());
      CHECK_EQ(graph->Size(), 2);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#code", "42"),
               true);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#altCode", "42"),
               true);
    }

    SUBCASE(
        "description-level predicates default for properties without QName") {
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "context": {
      "predicates": [
        "https://example.org#code",
        "https://example.org#altCode"
      ]
    },
    "properties": [
      {
        "literal": { "compute": { "xquery": "/item/value/text()" } }
      }
    ]
  }
})JSON";

      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE(graph.has_value());
      CHECK_EQ(graph->Size(), 2);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#code", "42"),
               true);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#altCode", "42"),
               true);
    }

    SUBCASE("computed predicates list expands whitespace-separated IRIs") {
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [
      {
        "context": {
          "predicates": {
            "compute": {
              "xquery": "string-join(('https://example.org#a', 'https://example.org#b'), ' ')"
            }
          }
        },
        "literal": { "compute": { "xquery": "/item/value/text()" } }
      }
    ]
  }
})JSON";

      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE(graph.has_value());
      CHECK_EQ(graph->Size(), 2);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#a", "42"),
               true);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#b", "42"),
               true);
    }
  }
}
