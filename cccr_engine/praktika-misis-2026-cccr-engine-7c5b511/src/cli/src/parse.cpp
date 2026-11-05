#include <pch.hpp>

#include <cccr/cli/parse.hpp>

#include <cccr/log/log.hpp>

#include <argparse/argparse.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#ifndef CCCR_PROJECT_VERSION
#define CCCR_PROJECT_VERSION "0.0.0"
#endif

namespace cccr::cli {

namespace {

[[nodiscard]] constexpr auto ParseInputFormat(std::string_view value)
    -> CliResult<InputFormat> {
  if (value == "xml") {
    return InputFormat::kXml;
  }

  if (value == "csv") {
    return InputFormat::kCsv;
  }

  return std::unexpected(CliError::kInvalidArgs);
}

[[nodiscard]] constexpr auto ParseResultFormat(std::string_view value)
    -> CliResult<sparql::ResultFormat> {
  if (value == "json") {
    return sparql::ResultFormat::kJson;
  }

  if (value == "xml") {
    return sparql::ResultFormat::kXml;
  }

  if (value == "html") {
    return sparql::ResultFormat::kHtml;
  }

  return std::unexpected(CliError::kInvalidArgs);
}

[[nodiscard]] constexpr auto ParseGraphFormat(std::string_view value)
    -> CliResult<GraphFormat> {
  if (value == "ntriples" || value == "nt") {
    return GraphFormat::kNTriples;
  }

  if (value == "turtle" || value == "ttl") {
    return GraphFormat::kTurtle;
  }

  if (value == "rdfxml" || value == "rdf" || value == "xml") {
    return GraphFormat::kRdfXml;
  }

  return std::unexpected(CliError::kInvalidArgs);
}

[[nodiscard]] auto ToPaths(const std::vector<std::string>& values)
    -> std::vector<std::filesystem::path> {
  std::vector<std::filesystem::path> paths;
  paths.reserve(values.size());
  for (const auto& value : values) {
    paths.emplace_back(value);
  }
  return paths;
}

}  // namespace

auto ParseArgs(int argc, char* argv[]) -> CliResult<ParsedCommand> {
  argparse::ArgumentParser program("cccr", CCCR_PROJECT_VERSION);
  program.add_description(
      "CCCR engine: map weakly structured input into an RDF graph");

  argparse::ArgumentParser run_cmd("run");
  run_cmd.add_description(
      "Apply a CCCR schema to XML/CSV input and emit RDF/XML");

  run_cmd.add_group("Input");
  run_cmd.add_argument("-s", "--schema")
      .required()
      .help("path to CCCR JSON schema");
  run_cmd.add_argument("-i", "--input")
      .required()
      .nargs(argparse::nargs_pattern::at_least_one)
      .help("one or more XML/CSV input paths");
  run_cmd.add_argument("--input-format")
      .help("force input kind for all inputs: xml or csv")
      .choices("xml", "csv");

  run_cmd.add_group("Knowledge");
  run_cmd.add_argument("--seed")
      .nargs(argparse::nargs_pattern::at_least_one)
      .help("N-Triples seed paths to merge (repeatable / multi-value)");

  run_cmd.add_group("Output");
  run_cmd.add_argument("-o", "--output")
      .help("RDF/XML output path (default: stdout)");

  argparse::ArgumentParser query_cmd("query");
  query_cmd.add_description(
      "Run a SPARQL SELECT query over one or more RDF graphs");

  query_cmd.add_group("Graph");
  query_cmd.add_argument("-g", "--graph")
      .required()
      .nargs(argparse::nargs_pattern::at_least_one)
      .help("one or more RDF graph paths (merged)");
  query_cmd.add_argument("--graph-format")
      .help("force graph syntax for all files: ntriples, turtle, or rdfxml")
      .choices("ntriples", "turtle", "rdfxml");

  query_cmd.add_group("Query");
  query_cmd.add_argument("-q", "--query")
      .required()
      .help("SPARQL query text, or path to a .rq / .sparql file");

  query_cmd.add_group("Format");
  query_cmd.add_argument("--format")
      .default_value(std::string{"json"})
      .help("result format: json, xml, or html")
      .choices("json", "xml", "html");

  query_cmd.add_group("Output");
  query_cmd.add_argument("-o", "--output")
      .help("result output path (default: stdout)");

  program.add_subparser(run_cmd);
  program.add_subparser(query_cmd);

  try {
    program.parse_args(argc, argv);
  } catch (const std::exception& err) {
    log::Error("{}", err.what());
    std::cerr << program;
    return std::unexpected(CliError::kParseFailed);
  }

  if (program.is_subcommand_used("run")) {
    RunOptions options;
    options.schema = run_cmd.get<std::string>("--schema");
    options.inputs = ToPaths(run_cmd.get<std::vector<std::string>>("--input"));
    if (run_cmd.is_used("--seed")) {
      options.seeds = ToPaths(run_cmd.get<std::vector<std::string>>("--seed"));
    }

    if (run_cmd.is_used("--output")) {
      options.output = run_cmd.get<std::string>("--output");
    }

    if (run_cmd.is_used("--input-format")) {
      auto format =
          ParseInputFormat(run_cmd.get<std::string>("--input-format"));
      if (!format) [[unlikely]] {
        return std::unexpected(format.error());
      }
      options.input_format = *format;
    }

    return options;
  }

  if (program.is_subcommand_used("query")) {
    QueryOptions options;
    options.graphs =
        ToPaths(query_cmd.get<std::vector<std::string>>("--graph"));
    options.query = query_cmd.get<std::string>("--query");
    if (query_cmd.is_used("--output")) {
      options.output = query_cmd.get<std::string>("--output");
    }

    if (query_cmd.is_used("--graph-format")) {
      auto graph_format =
          ParseGraphFormat(query_cmd.get<std::string>("--graph-format"));
      if (!graph_format) [[unlikely]] {
        return std::unexpected(graph_format.error());
      }
      options.graph_format = *graph_format;
    }

    auto format = ParseResultFormat(query_cmd.get<std::string>("--format"));
    if (!format) [[unlikely]] {
      return std::unexpected(format.error());
    }

    options.format = *format;
    return options;
  }

  std::cerr << program;
  return std::unexpected(CliError::kInvalidArgs);
}

}  // namespace cccr::cli
