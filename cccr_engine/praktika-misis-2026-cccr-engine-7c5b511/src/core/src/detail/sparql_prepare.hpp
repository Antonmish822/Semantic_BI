#pragma once

#include <memory_resource>
#include <string>
#include <string_view>

namespace cccr::detail {

/// @brief Prepends builtin rdf/rdfs/xsd PREFIX decls and normalizes PREFIX
///        IRI bases (append `#` when missing `/` or `#`).
/// @param query SPARQL query text
/// @param allocator PMR resource for the returned string
/// @return Prepared query text
[[nodiscard]] std::pmr::string PrepareSparqlQuery(
    std::string_view query,
    std::pmr::memory_resource* allocator = std::pmr::get_default_resource());

}  // namespace cccr::detail
