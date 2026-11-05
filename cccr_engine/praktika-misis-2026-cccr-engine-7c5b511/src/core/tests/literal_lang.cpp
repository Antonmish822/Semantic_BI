#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <string_view>
#include <variant>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<item><label>Привет</label></item>
)";

constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:label" },
        "literal": { "compute": { "xquery": "/item/label/text()" } },
        "language": "ru"
      }
    ]
  }
})JSON";

[[nodiscard]] bool HasLangLiteral(const cccr::rdf::Graph& graph,
                                  std::string_view subject,
                                  std::string_view predicate,
                                  std::string_view object,
                                  std::string_view language) {
  for (const auto& triple : graph.Triples()) {
    if (triple.subject.value != subject ||
        triple.predicate.value != predicate) {
      continue;
    }
    const auto* lit = std::get_if<cccr::rdf::Literal>(&triple.object);
    if (lit != nullptr && lit->value == object && lit->language &&
        *lit->language == language) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST_SUITE("cccr::engine::Run language") {
  TEST_CASE("cccr::engine::Run literal language tag") {
    SUBCASE("language is emitted on literal") {
      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE(graph.has_value());
      CHECK_EQ(HasLangLiteral(*graph, "https://example.org/item/1",
                              "https://example.org#label", "Привет", "ru"),
               true);

      const auto xml = cccr::rdf::ToRdfXml(*graph);
      CHECK_EQ(xml.contains("xml:lang=\"ru\""), true);
    }
  }
}
