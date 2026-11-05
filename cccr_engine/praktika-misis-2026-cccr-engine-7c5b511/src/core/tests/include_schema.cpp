#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string_view>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<item/>
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

TEST_SUITE("cccr::json include schema fragment") {
  TEST_CASE("cccr::json::LoadSchema property include") {
    SUBCASE("splices included property into the schema") {
      const auto dir =
          std::filesystem::temp_directory_path() / "cccr_schema_include";
      std::filesystem::create_directories(dir);
      const auto fragment = dir / "extra.json";
      {
        std::ofstream out(fragment, std::ios::binary);
        out << R"JSON({
  "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:label" },
  "literal": "from-fragment"
})JSON";
      }

      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [
      { "include": "extra.json" },
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:flag" },
        "literal": "ok"
      }
    ]
  }
})JSON";

      const auto schema =
          cccr::json::LoadSchema(kSchema, {.include_base = dir});
      REQUIRE(schema.has_value());
      CHECK_EQ(schema->description.properties.size(), 2);

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph =
          cccr::engine::Run(*schema, source, {.include_base = dir});
      REQUIRE(graph.has_value());
      CHECK_EQ(graph->Size(), 2);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#label", "from-fragment"),
               true);
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#flag", "ok"),
               true);

      std::filesystem::remove_all(dir);
    }

    SUBCASE("missing fragment fails load") {
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [ { "include": "missing.json" } ]
  }
})JSON";

      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE_FALSE(schema.has_value());
      CHECK_EQ(schema.error(), cccr::json::JsonError::kIncludeFailed);
    }
  }
}
