#pragma once

#include <cccr/context.hpp>
#include <cccr/rdf/graph.hpp>
#include <cccr/sparql/error.hpp>

#include <cstdint>
#include <memory_resource>
#include <string_view>

namespace cccr::sparql {

/// @brief Wire formats for multi-row SPARQL SELECT results.
enum class ResultFormat : uint8_t {
  /// @brief SPARQL Results JSON.
  kJson,
  /// @brief SPARQL Results XML.
  kXml,
  /// @brief HTML table rendering.
  kHtml,
};

/// @brief Runs a SPARQL query and serializes all result rows.
/// @details Uses Redland/rasqal result formatters. Unlike @ref SelectFirst,
///          this returns the full multi-row document (not used by in-pipeline
///          `compute.sparql`).
/// @param query SPARQL query text
/// @param graph Knowledge graph
/// @param format Output format
/// @param ctx Shared allocator context for the returned document
/// @return Formatted document, or SparqlError
[[nodiscard]] auto QueryFormatted(std::string_view query,
                                  const rdf::Graph& graph, ResultFormat format,
                                  Context ctx = {})
    -> SparqlResult<std::pmr::string>;

}  // namespace cccr::sparql
