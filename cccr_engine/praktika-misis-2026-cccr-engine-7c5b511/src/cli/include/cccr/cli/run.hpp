#pragma once

#include <cccr/cli/error.hpp>
#include <cccr/cli/options.hpp>

namespace cccr::cli {

/// @brief Runs the CCCR pipeline for @p options and writes RDF/XML.
/// @param options Parsed `run` options
/// @return Empty success or CliError
[[nodiscard]] auto ExecuteRun(const RunOptions& options) -> CliResult<void>;

/// @brief Runs a SPARQL query over merged N-Triples graphs.
/// @param options Parsed `query` options
/// @return Empty success or CliError
[[nodiscard]] auto ExecuteQuery(const QueryOptions& options) -> CliResult<void>;

}  // namespace cccr::cli
