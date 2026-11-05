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
<item><v>ok</v></item>
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

TEST_SUITE("cccr::eval include") {
  TEST_CASE("cccr::engine::Run include evaluator") {
    SUBCASE("filesystem-relative include loads literal text") {
      const auto dir =
          std::filesystem::temp_directory_path() / "cccr_include_test";
      std::filesystem::create_directories(dir);
      const auto included = dir / "label.txt";
      {
        std::ofstream out(included, std::ios::binary);
        out << "from-include";
      }

      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:label" },
        "literal": { "compute": { "include": "label.txt" } }
      }
    ]
  }
})JSON";

      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph =
          cccr::engine::Run(*schema, source, {.include_base = dir});
      REQUIRE(graph.has_value());
      CHECK_EQ(HasLiteralObject(*graph, "https://example.org/item/1",
                                "https://example.org#label", "from-include"),
               true);

      std::filesystem::remove_all(dir);
    }

    SUBCASE("missing include file fails evaluation") {
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:label" },
        "literal": { "compute": { "include": "missing-file.txt" } }
      }
    ]
  }
})JSON";

      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());

      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(*schema, source, {});
      REQUIRE_FALSE(graph.has_value());
      CHECK_EQ(graph.error(), cccr::engine::EngineError::kEvalFailed);
    }
  }
}
