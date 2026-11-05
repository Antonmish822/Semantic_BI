#pragma once

#include <cccr/context.hpp>
#include <cccr/rdf/graph.hpp>
#include <cccr/validate/error.hpp>

namespace cccr::validate {

/// @brief Checks that every triple in @p data respects `rdfs:range` in
///        @p ontology.
/// @details For each `(s, p, o)` in @p data, if ontology contains
///          `(p, rdfs:range, R)`:
///          - literal `o` and XSD `R` → @ref CheckLiteral
///          - IRI `o` and class `R` → require `(o, rdf:type, R)` in
///            data∪ontology (no RDFS entailment)
///          - otherwise → @ref ValidateError::kRangeViolation
///          Predicates with no `rdfs:range` are skipped.
/// @param data Emitted / instance triples to validate
/// @param ontology Schema / seed triples that declare ranges and types
/// @param ctx Shared allocator context for temporary lookup maps
/// @return Empty success or `ValidateError`
[[nodiscard]] auto CheckRdfsRange(const rdf::Graph& data,
                                  const rdf::Graph& ontology, Context ctx = {})
    -> ValidateResult<void>;

}  // namespace cccr::validate
