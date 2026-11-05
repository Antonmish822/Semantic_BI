#include <cccr/rdf/rdf.hpp>

#include <doctest/doctest.h>

#include <string_view>

TEST_SUITE("cccr::rdf::LoadRdf") {
  TEST_CASE("cccr::rdf::LoadRdf turtle and rdfxml") {
    constexpr std::string_view kTurtle = R"(
@prefix ex: <https://example.org#> .
ex:a ex:p "one" .
)";

    const auto turtle = cccr::rdf::LoadTurtle(kTurtle);
    REQUIRE(turtle.has_value());
    CHECK_EQ(turtle->Size(), 1);

    const auto rdfxml = cccr::rdf::ToRdfXml(*turtle);
    CHECK_EQ(rdfxml.empty(), false);

    const auto roundtrip = cccr::rdf::LoadRdfXml(rdfxml);
    REQUIRE(roundtrip.has_value());
    CHECK_EQ(roundtrip->Size(), 1);

    const auto forced =
        cccr::rdf::LoadRdf(rdfxml, cccr::rdf::RdfSyntax::kRdfXml);
    REQUIRE(forced.has_value());
    CHECK_EQ(forced->Size(), 1);
  }

  TEST_CASE("cccr::rdf::LoadNTriples still works") {
    constexpr std::string_view kNt =
        R"(<https://example.org/a> <https://example.org#p> "one" .)";
    const auto graph = cccr::rdf::LoadNTriples(kNt);
    REQUIRE(graph.has_value());
    CHECK_EQ(graph->Size(), 1);
  }
}
