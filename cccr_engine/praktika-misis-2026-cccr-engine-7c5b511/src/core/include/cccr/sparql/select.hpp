#pragma once

#include <cccr/context.hpp>
#include <cccr/rdf/graph.hpp>
#include <cccr/sparql/error.hpp>

#include <string>
#include <string_view>

namespace cccr::sparql {

/// @brief Runs a SPARQL `SELECT` against @p graph via Redland/rasqal.
/// @details Prepends built-in `rdf` / `rdfs` / `xsd` prefixes when absent.
///          PREFIX IRI bases that lack a trailing `#` or `/` get `#` appended
///          (same convention as CCCR namespace joining). Returns the first
///          binding of the first projected variable.
/// @param query SPARQL SELECT query
/// @param graph Knowledge graph
/// @param ctx Shared allocator context
/// @return First binding of the first projected variable, or SparqlError
[[nodiscard]] auto SelectFirst(std::string_view query, const rdf::Graph& graph,
                               Context ctx = {})
    -> SparqlResult<std::pmr::string>;

}  // namespace cccr::sparql
