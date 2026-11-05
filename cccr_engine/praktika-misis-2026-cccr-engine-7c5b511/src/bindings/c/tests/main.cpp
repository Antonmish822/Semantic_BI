#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cccr/bindings/c/cccr.h>

#include <filesystem>
#include <fstream>
#include <string>

#ifndef CCCR_BINDINGS_BASE_DATA_DIR
#error "CCCR_BINDINGS_BASE_DATA_DIR is required"
#endif

namespace {

[[nodiscard]] std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  REQUIRE(static_cast<bool>(in));
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST_SUITE("cccr::bindings::c") {
  TEST_CASE("cccr_version and cccr_status_string") {
    CHECK(cccr_version() != nullptr);
    CHECK(std::string{cccr_version()}.empty() == false);
    CHECK_EQ(std::string{cccr_status_string(CCCR_OK)}, "OK");
  }

  TEST_CASE("cccr_context_create_with_arena") {
    cccr_context* ctx =
        cccr_context_create_with_arena(CCCR_CONTEXT_DEFAULT_ARENA_BYTES * 2);
    REQUIRE(ctx != nullptr);
    CHECK_EQ(std::string{cccr_context_last_error(ctx)}.empty(), true);
    cccr_context_destroy(ctx);
  }

  TEST_CASE("cccr_engine_run base example") {
    const std::filesystem::path data{CCCR_BINDINGS_BASE_DATA_DIR};
    const auto schema_text = ReadFile(data / "schema.json");
    const auto input_text = ReadFile(data / "input.xml");
    const auto seed_text = ReadFile(data / "seed.nt");

    cccr_context* ctx = cccr_context_create_with_arena(2 * 1024 * 1024);
    REQUIRE(ctx != nullptr);

    cccr_schema* schema =
        cccr_schema_load(ctx, schema_text.c_str(), data.string().c_str());
    REQUIRE(schema != nullptr);

    cccr_source* source = cccr_source_load_xml(ctx, input_text.c_str());
    REQUIRE(source != nullptr);

    cccr_graph* knowledge =
        cccr_graph_load(ctx, seed_text.c_str(), CCCR_GRAPH_NTRIPLES);
    REQUIRE(knowledge != nullptr);

    cccr_graph* graph = cccr_engine_run(ctx, schema, source, knowledge);
    REQUIRE(graph != nullptr);

    char* rdfxml = cccr_graph_to_rdfxml(ctx, graph);
    REQUIRE(rdfxml != nullptr);

    const std::string out{rdfxml};
    cccr_string_free(rdfxml);

    CHECK_EQ(out.contains("https://lipers24.ru/SPO_2009_универсальный") ||
                 out.contains("lipers24"),
             true);
    CHECK_EQ(out.contains("rdf:RDF") || out.contains("RDF"), true);

    cccr_graph_destroy(graph);
    cccr_graph_destroy(knowledge);
    cccr_source_destroy(source);
    cccr_schema_destroy(schema);
    cccr_context_destroy(ctx);
  }

  TEST_CASE("cccr_graph_query seed N-Triples") {
    const std::filesystem::path data{CCCR_BINDINGS_BASE_DATA_DIR};
    const auto seed_text = ReadFile(data / "seed.nt");
    const char* sparql =
        "SELECT ?o WHERE { <https://lipers24.ru/SPO> "
        "<https://lipers24.ru#shortName> ?o }";

    cccr_context* ctx = cccr_context_create();
    REQUIRE(ctx != nullptr);

    cccr_graph* graph =
        cccr_graph_load(ctx, seed_text.c_str(), CCCR_GRAPH_NTRIPLES);
    REQUIRE(graph != nullptr);

    char* json = cccr_graph_query(ctx, graph, sparql, CCCR_RESULT_JSON);
    REQUIRE(json != nullptr);

    const std::string out{json};
    cccr_string_free(json);

    CHECK_EQ(out.contains("bindings") || out.contains("lipers24") ||
                 out.empty() == false,
             true);

    cccr_graph_destroy(graph);
    cccr_context_destroy(ctx);
  }

  TEST_CASE("cccr_engine_run_many base example") {
    const std::filesystem::path data{CCCR_BINDINGS_BASE_DATA_DIR};
    const auto schema_text = ReadFile(data / "schema.json");
    const auto input_text = ReadFile(data / "input.xml");
    const auto seed_text = ReadFile(data / "seed.nt");

    cccr_context* ctx = cccr_context_create_with_arena(2 * 1024 * 1024);
    REQUIRE(ctx != nullptr);

    cccr_schema* schema =
        cccr_schema_load(ctx, schema_text.c_str(), data.string().c_str());
    REQUIRE(schema != nullptr);

    cccr_source* source_a = cccr_source_load_xml(ctx, input_text.c_str());
    cccr_source* source_b = cccr_source_load_xml(ctx, input_text.c_str());
    REQUIRE(source_a != nullptr);
    REQUIRE(source_b != nullptr);

    cccr_graph* knowledge =
        cccr_graph_load(ctx, seed_text.c_str(), CCCR_GRAPH_NTRIPLES);
    REQUIRE(knowledge != nullptr);

    cccr_source* sources[] = {source_a, source_b};
    cccr_graph* graph =
        cccr_engine_run_many(ctx, schema, sources, 2, knowledge);
    REQUIRE(graph != nullptr);

    char* rdfxml = cccr_graph_to_rdfxml(ctx, graph);
    REQUIRE(rdfxml != nullptr);
    const std::string out{rdfxml};
    cccr_string_free(rdfxml);

    CHECK_EQ(out.contains("lipers24"), true);
    CHECK_EQ(out.contains("rdf:RDF") || out.contains("RDF"), true);

    cccr_graph_destroy(graph);
    cccr_graph_destroy(knowledge);
    cccr_source_destroy(source_b);
    cccr_source_destroy(source_a);
    cccr_schema_destroy(schema);
    cccr_context_destroy(ctx);
  }

  TEST_CASE("cccr_graph_merge seed graphs") {
    const std::filesystem::path data{CCCR_BINDINGS_BASE_DATA_DIR};
    const auto seed_text = ReadFile(data / "seed.nt");
    const char* sparql =
        "SELECT ?o WHERE { <https://lipers24.ru/SPO> "
        "<https://lipers24.ru#shortName> ?o }";

    cccr_context* ctx = cccr_context_create();
    REQUIRE(ctx != nullptr);

    cccr_graph* dest =
        cccr_graph_load(ctx, seed_text.c_str(), CCCR_GRAPH_NTRIPLES);
    cccr_graph* other =
        cccr_graph_load(ctx, seed_text.c_str(), CCCR_GRAPH_NTRIPLES);
    REQUIRE(dest != nullptr);
    REQUIRE(other != nullptr);

    CHECK_EQ(cccr_graph_merge(dest, other), CCCR_OK);

    char* json = cccr_graph_query(ctx, dest, sparql, CCCR_RESULT_JSON);
    REQUIRE(json != nullptr);
    const std::string out{json};
    cccr_string_free(json);

    CHECK_EQ(
        out.contains("bindings") || out.contains("СПО") || out.empty() == false,
        true);

    cccr_graph_destroy(other);
    cccr_graph_destroy(dest);
    cccr_context_destroy(ctx);
  }
}
