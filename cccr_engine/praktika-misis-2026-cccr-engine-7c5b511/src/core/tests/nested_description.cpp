#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <string_view>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<root>
  <org id="42">
    <name>Acme</name>
    <dept>
      <title>Research</title>
    </dept>
  </org>
</root>
)";

constexpr std::string_view kSchema = R"JSON({
  "description": {
    "context": {
      "subject": { "compute": { "xquery": "/root/org" } }
    },
    "about": {
      "compute": {
        "xquery": "concat('https://example.org/org/', @id)"
      }
    },
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:name" },
        "literal": { "compute": { "xquery": "./name/text()" } }
      },
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:hasDepartment" },
        "description": {
          "about": {
            "compute": {
              "xquery": "concat('https://example.org/org/', @id, '/dept')"
            }
          },
          "properties": [
            {
              "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:title" },
              "literal": { "compute": { "xquery": "./dept/title/text()" } }
            }
          ]
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

TEST_SUITE("cccr::engine::Run nested Description") {
  TEST_CASE("cccr::engine::Run nested Description object") {
    SUBCASE("parent subject, child subject, and linking triple") {
      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE(graph.has_value());

      constexpr std::string_view kParent = "https://example.org/org/42";
      constexpr std::string_view kChild = "https://example.org/org/42/dept";

      CHECK_EQ(graph->Size(), 3);
      CHECK_EQ(
          HasLiteralObject(*graph, kParent, "https://example.org#name", "Acme"),
          true);
      CHECK_EQ(HasIriObject(*graph, kParent,
                            "https://example.org#hasDepartment", kChild),
               true);
      CHECK_EQ(HasLiteralObject(*graph, kChild, "https://example.org#title",
                                "Research"),
               true);
    }
  }
}
