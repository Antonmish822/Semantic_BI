#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/validate/validate.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <optional>
#include <string_view>
#include <variant>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<root/>
)";

}  // namespace

TEST_SUITE("cccr::validate::CheckLiteral") {
  TEST_CASE("cccr::validate::CheckLiteral") {
    SUBCASE("integer accepts digits") {
      const auto result = cccr::validate::CheckLiteral(
          "2018", "integer", std::nullopt, std::nullopt);
      CHECK_EQ(result.has_value(), true);
    }

    SUBCASE("integer rejects non-numeric") {
      const auto result = cccr::validate::CheckLiteral(
          "abc", "xsd:integer", std::nullopt, std::nullopt);
      REQUIRE_FALSE(result.has_value());
      CHECK_EQ(result.error(), cccr::validate::ValidateError::kTypeMismatch);
    }

    SUBCASE("positiveInteger rejects zero") {
      const auto result = cccr::validate::CheckLiteral(
          "0", "positiveInteger", std::nullopt, std::nullopt);
      REQUIRE_FALSE(result.has_value());
      CHECK_EQ(result.error(), cccr::validate::ValidateError::kTypeMismatch);
    }

    SUBCASE("range minInclusive rejects below") {
      const auto result = cccr::validate::CheckLiteral(
          "2009", "integer", std::string_view{"2010"}, std::nullopt);
      REQUIRE_FALSE(result.has_value());
      CHECK_EQ(result.error(), cccr::validate::ValidateError::kOutOfRange);
    }

    SUBCASE("range accepts inclusive bounds") {
      const auto result = cccr::validate::CheckLiteral(
          "2010", "http://www.w3.org/2001/XMLSchema#integer",
          std::string_view{"2010"}, std::string_view{"2020"});
      CHECK_EQ(result.has_value(), true);
    }

    SUBCASE("date lexical form") {
      const auto ok = cccr::validate::CheckLiteral("2018-10-28", "date",
                                                   std::nullopt, std::nullopt);
      CHECK_EQ(ok.has_value(), true);
      const auto bad = cccr::validate::CheckLiteral("2018/10/28", "date",
                                                    std::nullopt, std::nullopt);
      REQUIRE_FALSE(bad.has_value());
      CHECK_EQ(bad.error(), cccr::validate::ValidateError::kTypeMismatch);
    }

    SUBCASE("unsupported datatype") {
      const auto result = cccr::validate::CheckLiteral(
          "x", "duration", std::nullopt, std::nullopt);
      REQUIRE_FALSE(result.has_value());
      CHECK_EQ(result.error(),
               cccr::validate::ValidateError::kUnsupportedDatatype);
    }

    SUBCASE("nonNegativeInteger and anyURI") {
      CHECK_EQ(cccr::validate::CheckLiteral("0", "nonNegativeInteger",
                                            std::nullopt, std::nullopt)
                   .has_value(),
               true);
      CHECK_EQ(cccr::validate::CheckLiteral("-1", "nonNegativeInteger",
                                            std::nullopt, std::nullopt)
                   .has_value(),
               false);
      CHECK_EQ(cccr::validate::CheckLiteral("https://example.org/a", "anyURI",
                                            std::nullopt, std::nullopt)
                   .has_value(),
               true);
      CHECK_EQ(cccr::validate::KnownXsdDatatype("xsd:dateTime"), true);
      CHECK_EQ(cccr::validate::CheckLiteral("2018-10-28T12:00:00Z", "dateTime",
                                            std::nullopt, std::nullopt)
                   .has_value(),
               true);
    }
  }
}

TEST_SUITE("cccr::engine::Run validation") {
  TEST_CASE("cccr::engine::Run literal constraints") {
    SUBCASE("valid gYear with range emits typed literal") {
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/p/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:year" },
        "literal": "2018",
        "datatype": "http://www.w3.org/2001/XMLSchema#gYear",
        "minInclusive": "2000",
        "maxInclusive": "2030"
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
      REQUIRE_EQ(graph->Size(), 1);

      const auto& triple = graph->Triples().front();
      CHECK_EQ(triple.subject.value, "https://example.org/p/1");
      const auto* lit = std::get_if<cccr::rdf::Literal>(&triple.object);
      REQUIRE(lit != nullptr);
      CHECK_EQ(lit->value, "2018");
      REQUIRE(lit->datatype.has_value());
      CHECK_EQ(*lit->datatype, "http://www.w3.org/2001/XMLSchema#gYear");
    }

    SUBCASE("out of range fails the run") {
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/p/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:year" },
        "literal": "1999",
        "datatype": "gYear",
        "minInclusive": "2000"
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
      CHECK_EQ(graph.error(), cccr::engine::EngineError::kValidationFailed);
    }
  }
}

TEST_SUITE("cccr::validate::CheckRdfsRange") {
  TEST_CASE("cccr::engine::Run rdfs:range") {
    SUBCASE("literal range accepts matching datatype") {
      constexpr std::string_view kSeed = R"(
<https://example.org#year> <http://www.w3.org/2000/01/rdf-schema#range> <http://www.w3.org/2001/XMLSchema#gYear> .
)";
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/p/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:year" },
        "literal": "2018",
        "datatype": "http://www.w3.org/2001/XMLSchema#gYear"
      }
    ]
  }
})JSON";

      const auto knowledge = cccr::rdf::LoadNTriples(kSeed);
      REQUIRE(knowledge.has_value());
      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());
      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(
          *schema, source, {.knowledge = *knowledge});
      REQUIRE(graph.has_value());
      CHECK_EQ(graph->Size(), 1);
    }

    SUBCASE("literal range rejects bad lexical form") {
      constexpr std::string_view kSeed = R"(
<https://example.org#year> <http://www.w3.org/2000/01/rdf-schema#range> <http://www.w3.org/2001/XMLSchema#gYear> .
)";
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/p/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:year" },
        "literal": "not-a-year"
      }
    ]
  }
})JSON";

      const auto knowledge = cccr::rdf::LoadNTriples(kSeed);
      REQUIRE(knowledge.has_value());
      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());
      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(
          *schema, source, {.knowledge = *knowledge});
      REQUIRE_FALSE(graph.has_value());
      CHECK_EQ(graph.error(), cccr::engine::EngineError::kValidationFailed);
    }

    SUBCASE("class range requires rdf:type") {
      constexpr std::string_view kSeed = R"(
<https://example.org#level> <http://www.w3.org/2000/01/rdf-schema#range> <https://example.org#EducationLevel> .
<https://example.org/SPO> <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> <https://example.org#EducationLevel> .
)";
      constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/p/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:level" },
        "URI": "https://example.org/SPO"
      }
    ]
  }
})JSON";

      const auto knowledge = cccr::rdf::LoadNTriples(kSeed);
      REQUIRE(knowledge.has_value());
      const auto schema = cccr::json::LoadSchema(kSchema);
      REQUIRE(schema.has_value());
      cccr::xml::XmlSource source;
      REQUIRE(source.Load(kXml).has_value());
      const auto graph = cccr::engine::Run(
          *schema, source, {.knowledge = *knowledge});
      REQUIRE(graph.has_value());
    }
  }
}
