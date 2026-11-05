#pragma once

#include <cccr/ast/types.hpp>

#include <cccr/context.hpp>

#include <string>

namespace cccr::eval {

/// @brief Builds a predicate IRI from a CCCR QName.
/// @param qname Schema QName
/// @param ctx Shared allocator context
/// @return Absolute IRI string
[[nodiscard]] std::pmr::string QNameToIri(const ast::QName& qname,
                                          Context ctx = {});

}  // namespace cccr::eval
