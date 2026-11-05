#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <array>
#include <functional>
#include <memory_resource>
#include <string_view>

TEST_SUITE("cccr::rdf::Graph::MergeFrom") {
  TEST_CASE("cccr::rdf::Graph::MergeFrom merges triples") {
    constexpr std::string_view kA =
        R"(<https://example.org/a> <https://example.org#p> "one" .)";
    constexpr std::string_view kB =
        R"(<https://example.org/b> <https://example.org#p> "two" .)";

    auto left = cccr::rdf::LoadNTriples(kA);
    auto right = cccr::rdf::LoadNTriples(kB);
    REQUIRE(left.has_value());
    REQUIRE(right.has_value());

    left->MergeFrom(*right);
    CHECK_EQ(left->Size(), 2);
  }
}

TEST_SUITE("cccr::rdf::LoadNTriples") {
  TEST_CASE("cccr::rdf::LoadNTriples range overload") {
    constexpr std::string_view kA =
        R"(<https://example.org/a> <https://example.org#p> "one" .)";
    constexpr std::string_view kB =
        R"(<https://example.org/b> <https://example.org#p> "two" .)";

    const auto docs = std::array{kA, kB};
    const auto graph = cccr::rdf::LoadNTriples(docs);
    REQUIRE(graph.has_value());
    CHECK_EQ(graph->Size(), 2);
  }
}

TEST_SUITE("cccr::engine::Run") {
  TEST_CASE("cccr::engine::Run range overload") {
    constexpr std::string_view kXml = R"(<?xml version="1.0"?>
<root><item>x</item></root>
)";
    constexpr std::string_view kSchema = R"JSON({
      "description": {
        "context": { "subject": { "compute": { "xquery": "/root" } } },
        "about": { "compute": { "xquery": "'https://example.org/item'" } },
        "properties": [
          {
            "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:label" },
            "literal": { "compute": { "xquery": "string(item)" } }
          }
        ]
      }
    })JSON";

    std::pmr::monotonic_buffer_resource arena(64 * 1024);
    cccr::Context ctx{.allocator = &arena};

    const auto schema = cccr::json::LoadSchema(kSchema, {.ctx = ctx});
    REQUIRE(schema.has_value());

    cccr::xml::XmlSource first(ctx);
    cccr::xml::XmlSource second(ctx);
    REQUIRE(first.Load(kXml));
    REQUIRE(second.Load(kXml));

    const auto refs = std::array{std::ref(first), std::ref(second)};
    const auto graph =
        cccr::engine::Run(*schema, refs, cccr::engine::RunParams{.ctx = ctx});
    REQUIRE(graph.has_value());
    CHECK_EQ(graph->Size(), 2);
  }
}
