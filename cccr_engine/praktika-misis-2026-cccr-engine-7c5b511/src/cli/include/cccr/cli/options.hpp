#pragma once

#include <cccr/cli/error.hpp>
#include <cccr/sparql/results.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace cccr::cli {

/// @brief Input document kind for @ref RunOptions.
enum class InputFormat : uint8_t {
  /// @brief Infer from each path's extension (`.csv` → CSV, else XML).
  kAuto,
  /// @brief Force XML for all inputs.
  kXml,
  /// @brief Force CSV for all inputs.
  kCsv,
};

/// @brief Graph file syntax for @ref QueryOptions.
enum class GraphFormat : uint8_t {
  /// @brief Infer from each path's extension (`.ttl` / `.rdf` / `.nt`, …).
  kAuto,
  /// @brief Force N-Triples for all graphs.
  kNTriples,
  /// @brief Force Turtle for all graphs.
  kTurtle,
  /// @brief Force RDF/XML for all graphs.
  kRdfXml,
};

/// @brief Options for the `run` subcommand.
struct RunOptions {
  /// @brief Path to CCCR JSON schema.
  std::filesystem::path schema;

  /// @brief Input document paths (XML and/or CSV).
  std::vector<std::filesystem::path> inputs;

  /// @brief Optional N-Triples seed paths (merged into knowledge).
  std::vector<std::filesystem::path> seeds;

  /// @brief Output path; empty writes RDF/XML to stdout.
  std::filesystem::path output;

  /// @brief Input format override.
  InputFormat input_format = InputFormat::kAuto;
};

/// @brief Options for the `query` subcommand.
struct QueryOptions {
  /// @brief RDF graph paths (merged; N-Triples / Turtle / RDF/XML).
  std::vector<std::filesystem::path> graphs;

  /// @brief SPARQL query text, or a path to a query file.
  std::string query;

  /// @brief Output path; empty writes to stdout.
  std::filesystem::path output;

  /// @brief Result serialization format.
  sparql::ResultFormat format = sparql::ResultFormat::kJson;

  /// @brief Graph syntax override (default: infer per path).
  GraphFormat graph_format = GraphFormat::kAuto;
};

/// @brief Parsed CLI command payload.
using ParsedCommand = std::variant<RunOptions, QueryOptions>;

}  // namespace cccr::cli
