#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/sparql/sparql.hpp>
#include <cccr/xml/xml.hpp>

#include <doctest/doctest.h>

#include <cstddef>
#include <memory_resource>
#include <string_view>

namespace {

constexpr std::string_view kXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<root/>
)";

constexpr std::string_view kSchema = R"JSON({
  "description": {
    "about": "https://example.org/item/1",
    "properties": [
      {
        "QName": { "NameSpace": "https://example.org", "PrefixedName": "ex:label" },
        "literal": "hello"
      }
    ]
  }
})JSON";

constexpr std::string_view kNtriples = R"(
<https://example.org/a> <https://example.org#p> "one" .
<https://example.org/b> <https://example.org#p> "two" .
)";

class CountingResource final : public std::pmr::memory_resource {
public:
  [[nodiscard]] size_t Allocations() const noexcept { return allocations_; }
  [[nodiscard]] size_t Bytes() const noexcept { return bytes_; }

protected:
  auto do_allocate(size_t bytes, size_t alignment) -> void* override {
    ++allocations_;
    bytes_ += bytes;
    return std::pmr::new_delete_resource()->allocate(bytes, alignment);
  }

  void do_deallocate(void* p, size_t bytes, size_t alignment) override {
    std::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
  }

  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

private:
  size_t allocations_ = 0;
  size_t bytes_ = 0;
};

}  // namespace

TEST_SUITE("cccr::Context allocator") {
  TEST_CASE("cccr::engine::Run uses Context::allocator") {
    CountingResource resource;

    const auto schema = cccr::json::LoadSchema(kSchema);
    REQUIRE(schema.has_value());

    cccr::xml::XmlSource source{{.allocator = &resource}};
    REQUIRE(source.Load(kXml).has_value());
    const auto graph =
        cccr::engine::Run(*schema, source, {.ctx = {.allocator = &resource}});
    REQUIRE(graph.has_value());
    CHECK_EQ(graph->Allocator(), &resource);
    CHECK_GT(resource.Allocations(), 0);
    CHECK_EQ(graph->Size(), 1);
  }

  TEST_CASE("cccr::rdf::ToRdfXml uses Graph::Allocator") {
    CountingResource resource;

    const auto graph =
        cccr::rdf::LoadNTriples(kNtriples, {.allocator = &resource});
    REQUIRE(graph.has_value());
    CHECK_EQ(graph->Allocator(), &resource);

    const size_t before = resource.Allocations();
    const auto xml = cccr::rdf::ToRdfXml(*graph);
    CHECK_EQ(xml.get_allocator().resource(), &resource);
    CHECK_GT(resource.Allocations(), before);
    CHECK_EQ(xml.contains("https://example.org/a"), true);
  }

  TEST_CASE("cccr::sparql::QueryFormatted uses Context::allocator") {
    CountingResource resource;

    const auto graph =
        cccr::rdf::LoadNTriples(kNtriples, {.allocator = &resource});
    REQUIRE(graph.has_value());

    constexpr std::string_view kQuery =
        "SELECT ?s ?o WHERE { ?s <https://example.org#p> ?o }";
    const size_t before = resource.Allocations();
    const auto json = cccr::sparql::QueryFormatted(
        kQuery, *graph, cccr::sparql::ResultFormat::kJson,
        {.allocator = &resource});
    REQUIRE(json.has_value());
    CHECK_EQ(json->get_allocator().resource(), &resource);
    CHECK_GT(resource.Allocations(), before);
    CHECK_EQ(json->contains("one"), true);
  }
}
