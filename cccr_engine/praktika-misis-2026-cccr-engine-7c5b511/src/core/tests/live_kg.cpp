#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <string_view>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<item/>
)";

constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/SPO",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:shortName" },
        "literal": "СПО"
      },
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:lookup" },
        "URI": {
          "compute": {
            "sparql": "SELECT ?uri WHERE { ?uri <https://example.org#shortName> \"СПО\" }"
          }
        }
      }
    ]
  }
})JSON";

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

}  // namespace

TEST_SUITE("cccr::engine::Run live knowledge") {
  TEST_CASE("cccr::engine::Run live KG for SPARQL") {
    SUBCASE("later sparql sees triples emitted earlier in the same run") {
      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE(graph.has_value());
      CHECK_EQ(graph->Size(), 2);
      CHECK_EQ(
          HasIriObject(*graph, "https://example.org/SPO",
                       "https://example.org#lookup", "https://example.org/SPO"),
          true);
    }
  }
}
