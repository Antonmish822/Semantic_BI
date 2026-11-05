#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <string_view>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<root>
  <row id="1"><cell>alpha</cell></row>
  <row id="2"><cell>beta</cell></row>
</root>
)";

constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/doc",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:cells" },
        "literal": {
          "compute": {
            "xquery": [ "/root/row", "./cell/text()" ]
          }
        }
      }
    ]
  }
})JSON";

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

TEST_SUITE("cccr::eval dual xquery") {
  TEST_CASE("cccr::engine::Run dual xquery array") {
    SUBCASE("select nodes then string-value on each") {
      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE(graph.has_value());
      CHECK_EQ(graph->Size(), 1);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/doc",
                                "https://example.org#cells", "alpha beta"),
               true);
    }
  }
}
