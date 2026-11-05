#include <cccr/cli/run.hpp>

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

#ifndef CCCR_CLI_BASE_DATA_DIR
#error "CCCR_CLI_BASE_DATA_DIR is required"
#endif

TEST_SUITE("cccr::cli::ExecuteRun") {
  TEST_CASE("cccr::cli::ExecuteRun base example") {
    const std::filesystem::path data{CCCR_CLI_BASE_DATA_DIR};
    const auto out_path =
        std::filesystem::temp_directory_path() / "cccr_cli_base_out.rdf";

    cccr::cli::RunOptions options;
    options.schema = data / "schema.json";
    options.inputs = {data / "input.xml"};
    options.seeds = {data / "seed.nt"};
    options.output = out_path;

    const auto result = cccr::cli::ExecuteRun(options);
    REQUIRE(result.has_value());

    std::ifstream in(out_path, std::ios::binary);
    REQUIRE(static_cast<bool>(in));
    const std::string rdfxml((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
    CHECK_EQ(rdfxml.contains("https://lipers24.ru/SPO_2009_универсальный") ||
                 rdfxml.contains("lipers24"),
             true);
    CHECK_EQ(rdfxml.contains("rdf:RDF") || rdfxml.contains("RDF"), true);

    std::filesystem::remove(out_path);
  }
}

TEST_SUITE("cccr::cli::ExecuteQuery") {
  TEST_CASE("cccr::cli::ExecuteQuery loads RDF/XML from run output") {
    const std::filesystem::path data{CCCR_CLI_BASE_DATA_DIR};
    const auto rdf_path =
        std::filesystem::temp_directory_path() / "cccr_cli_query_in.rdf";
    const auto out_path =
        std::filesystem::temp_directory_path() / "cccr_cli_query_out.json";

    cccr::cli::RunOptions run;
    run.schema = data / "schema.json";
    run.inputs = {data / "input.xml"};
    run.seeds = {data / "seed.nt"};
    run.output = rdf_path;
    REQUIRE(cccr::cli::ExecuteRun(run).has_value());

    cccr::cli::QueryOptions query;
    query.graphs = {rdf_path};
    query.graph_format = cccr::cli::GraphFormat::kRdfXml;
    query.query = "SELECT ?s WHERE { ?s ?p ?o } LIMIT 1";
    query.format = cccr::sparql::ResultFormat::kJson;
    query.output = out_path;

    const auto result = cccr::cli::ExecuteQuery(query);
    REQUIRE(result.has_value());

    std::ifstream in(out_path, std::ios::binary);
    REQUIRE(static_cast<bool>(in));
    const std::string json((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());
    CHECK_EQ(json.contains("bindings") || json.contains("lipers24"), true);

    std::filesystem::remove(rdf_path);
    std::filesystem::remove(out_path);
  }

  TEST_CASE("cccr::cli::ExecuteQuery loads N-Triples seed by extension") {
    const std::filesystem::path data{CCCR_CLI_BASE_DATA_DIR};

    cccr::cli::QueryOptions query;
    query.graphs = {data / "seed.nt"};
    query.query =
        "SELECT ?o WHERE { <https://lipers24.ru/SPO> "
        "<https://lipers24.ru#shortName> ?o }";
    query.format = cccr::sparql::ResultFormat::kJson;

    const auto result = cccr::cli::ExecuteQuery(query);
    REQUIRE(result.has_value());
  }
}
