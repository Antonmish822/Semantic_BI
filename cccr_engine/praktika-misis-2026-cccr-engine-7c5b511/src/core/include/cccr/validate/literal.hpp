#pragma once

#include <cccr/validate/error.hpp>

#include <optional>
#include <string_view>

namespace cccr::validate {

/// @brief Returns whether @p datatype is a supported XSD lexical type.
/// @param datatype XSD local name, CURIE, or IRI
/// @return `true` when @ref CheckLiteral can type-check it
[[nodiscard]] bool KnownXsdDatatype(std::string_view datatype) noexcept;

/// @brief Checks that @p value conforms to @p datatype and optional bounds.
/// @details Supports common XSD lexical forms including `string`, integer
///          family, `decimal`/`double`/`float`, `boolean`, `date`/`dateTime`/
///          `time`/`gYear`, and `anyURI`. Datatype may be a local name or IRI.
/// @param value Lexical literal value
/// @param datatype XSD datatype local name or IRI (empty → no type check)
/// @param min_inclusive Optional inclusive lower bound (lexical compare for
///                      strings; numeric for numeric types; lexical for dates)
/// @param max_inclusive Optional inclusive upper bound
/// @return Empty success or `ValidateError`
[[nodiscard]] auto CheckLiteral(std::string_view value,
                                std::string_view datatype,
                                std::optional<std::string_view> min_inclusive,
                                std::optional<std::string_view> max_inclusive)
    -> ValidateResult<void>;

}  // namespace cccr::validate
