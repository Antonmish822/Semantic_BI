#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <string_view>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<item><code>СПО</code></item>
)";

constexpr std::string_view kSeed = R"(
<https://example.org/SPO> <https://example.org#shortName> "СПО" .
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

}  // namespace

TEST_SUITE("cccr::eval sparql context") {
  TEST_CASE("cccr::engine::Run sparql-local context") {
    SUBCASE("compute.context pseudos override for sparql substitute") {
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:level" },
        "URI": {
          "compute": {
            "sparql": "SELECT ?uri WHERE { ?uri <https://example.org#shortName> {{code}} }",
            "context": {
              "code": { "compute": { "xquery": "/item/code/text()" } }
            }
          }
        }
      }
    ]
  }
})JSON";

      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      const auto knowledge = cccr::rdf::LoadNTriples(kSeed);
      REQUIRE(knowledge.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph =
          cccr::engine::Run(*schema, source, {.knowledge = *knowledge});
      REQUIRE(graph.has_value());
      CHECK_EQ(
          HasIriObject(*graph, "https://example.org/item/1",
                       "https://example.org#level", "https://example.org/SPO"),
          true);
    }
  }
}
