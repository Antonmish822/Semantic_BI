#include <cccr/cli/parse.hpp>

#include <doctest/doctest.h>

#include <string>
#include <variant>
#include <vector>

namespace {

[[nodiscard]] auto MakeArgv(std::vector<std::string>& storage)
    -> std::vector<char*> {
  std::vector<char*> argv;
  argv.reserve(storage.size());
  for (auto& arg : storage) {
    argv.push_back(arg.data());
  }
  return argv;
}

}  // namespace

TEST_SUITE("cccr::cli::ParseArgs") {
  TEST_CASE("cccr::cli::ParseArgs run options") {
    std::vector<std::string> args = {
        "cccr",   "run",   "-s",    "schema.json",    "-i",  "a.xml", "b.xml",
        "--seed", "s1.nt", "s2.nt", "--input-format", "xml", "-o",    "out.rdf",
    };
    auto argv = MakeArgv(args);
    const auto parsed =
        cccr::cli::ParseArgs(static_cast<int>(argv.size()), argv.data());
    REQUIRE(parsed.has_value());
    REQUIRE(std::holds_alternative<cccr::cli::RunOptions>(*parsed));

    const auto& options = std::get<cccr::cli::RunOptions>(*parsed);
    CHECK_EQ(options.schema.string(), "schema.json");
    CHECK_EQ(options.inputs.size(), 2);
    CHECK_EQ(options.seeds.size(), 2);
    CHECK_EQ(options.output.string(), "out.rdf");
    CHECK_EQ(options.input_format, cccr::cli::InputFormat::kXml);
  }

  TEST_CASE("cccr::cli::ParseArgs query options") {
    std::vector<std::string> args = {
        "cccr",
        "query",
        "-g",
        "a.nt",
        "b.ttl",
        "-q",
        "SELECT * WHERE { ?s ?p ?o }",
        "--format",
        "xml",
        "--graph-format",
        "turtle",
    };
    auto argv = MakeArgv(args);
    const auto parsed =
        cccr::cli::ParseArgs(static_cast<int>(argv.size()), argv.data());
    REQUIRE(parsed.has_value());
    REQUIRE(std::holds_alternative<cccr::cli::QueryOptions>(*parsed));

    const auto& options = std::get<cccr::cli::QueryOptions>(*parsed);
    CHECK_EQ(options.graphs.size(), 2);
    CHECK_EQ(options.format, cccr::sparql::ResultFormat::kXml);
    CHECK_EQ(options.graph_format, cccr::cli::GraphFormat::kTurtle);
  }

  TEST_CASE("cccr::cli::ParseArgs rejects bad input-format") {
    std::vector<std::string> args = {
        "cccr", "run", "-s", "s.json", "-i", "a.xml", "--input-format", "json",
    };
    auto argv = MakeArgv(args);
    const auto parsed =
        cccr::cli::ParseArgs(static_cast<int>(argv.size()), argv.data());
    CHECK_EQ(parsed.has_value(), false);
  }
}
