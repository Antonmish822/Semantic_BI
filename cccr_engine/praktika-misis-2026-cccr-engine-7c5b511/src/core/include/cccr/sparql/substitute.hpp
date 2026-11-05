#pragma once

#include <cccr/context.hpp>

#include <string>
#include <string_view>
#include <unordered_map>

namespace cccr::sparql {

/// @brief Pseudo-variable bindings (`{{name}}` → value).
using PseudoMap = std::pmr::unordered_map<std::pmr::string, std::pmr::string>;

/// @brief Substitutes `{{name}}` placeholders using @p variables.
/// @details Values are inserted as SPARQL string literals (quoted / escaped)
///          unless the placeholder is already adjacent to quotes in the query.
/// @param text Query text possibly containing `{{name}}`
/// @param variables Pseudo-variable map
/// @param ctx Shared allocator context
/// @return Query with substitutions applied
[[nodiscard]] std::pmr::string SubstitutePseudos(std::string_view text,
                                                 const PseudoMap& variables,
                                                 Context ctx = {});

}  // namespace cccr::sparql
