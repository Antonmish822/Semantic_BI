#include <cccr/rdf/rdf.hpp>
#include <cccr/sparql/sparql.hpp>

#include <doctest/doctest.h>

#include <string_view>

TEST_SUITE("cccr::sparql::QueryFormatted") {
  TEST_CASE("cccr::sparql::QueryFormatted multi-row formats") {
    constexpr std::string_view kNtriples = R"(
<https://example.org/a> <https://example.org#p> "one" .
<https://example.org/b> <https://example.org#p> "two" .
)";

    const auto graph = cccr::rdf::LoadNTriples(kNtriples);
    REQUIRE(graph.has_value());

    constexpr std::string_view kQuery =
        "SELECT ?s ?o WHERE { ?s <https://example.org#p> ?o }";

    SUBCASE("json") {
      const auto json = cccr::sparql::QueryFormatted(
          kQuery, *graph, cccr::sparql::ResultFormat::kJson);
      REQUIRE(json.has_value());
      CHECK_EQ(json->contains("https://example.org/a"), true);
      CHECK_EQ(json->contains("https://example.org/b"), true);
      CHECK_EQ(json->contains("one"), true);
      CHECK_EQ(json->contains("two"), true);
    }

    SUBCASE("xml") {
      const auto xml = cccr::sparql::QueryFormatted(
          kQuery, *graph, cccr::sparql::ResultFormat::kXml);
      REQUIRE(xml.has_value());
      CHECK_EQ(xml->contains("https://example.org/a"), true);
      CHECK_EQ(xml->contains("sparql"), true);
    }

    SUBCASE("html") {
      const auto html = cccr::sparql::QueryFormatted(
          kQuery, *graph, cccr::sparql::ResultFormat::kHtml);
      REQUIRE(html.has_value());
      CHECK_EQ(html->empty(), false);
      CHECK_EQ(html->contains("https://example.org/a") ||
                   html->contains("one"),
               true);
    }
  }
}
