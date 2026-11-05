#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <string_view>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<root>
  <org id="1">
    <dept id="a"><title>Alpha</title></dept>
    <dept id="b"><title>Beta</title></dept>
  </org>
</root>
)";

constexpr std::string_view kSchema = R"JSON({
  "description": {
    "context": {
      "subject": { "compute": { "xquery": "/root/org" } }
    },
    "about": {
      "compute": { "xquery": "concat('https://example.org/org/', @id)" }
    },
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:hasDepartment" },
        "description": {
          "context": {
            "subject": { "compute": { "xquery": "./dept" } }
          },
          "about": {
            "compute": {
              "xquery": "concat('https://example.org/dept/', @id)"
            }
          },
          "properties": [
            {
              "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:title" },
              "literal": { "compute": { "xquery": "./title/text()" } }
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

TEST_SUITE("cccr::engine::Run nested subject") {
  TEST_CASE("cccr::engine::Run nested Description subject rebind") {
    SUBCASE("nested context.subject selects child nodes") {
      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE(graph.has_value());

      CHECK_EQ(graph->Size(), 4);
      CHECK_EQ(HasIriObject(*graph, "https://example.org/org/1",
                            "https://example.org#hasDepartment",
                            "https://example.org/dept/a"),
               true);
      CHECK_EQ(HasIriObject(*graph, "https://example.org/org/1",
                            "https://example.org#hasDepartment",
                            "https://example.org/dept/b"),
               true);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/dept/a",
                                "https://example.org#title", "Alpha"),
               true);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/dept/b",
                                "https://example.org#title", "Beta"),
               true);
    }
  }
}
