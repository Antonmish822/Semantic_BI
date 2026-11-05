#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::validate {

/// @brief Errors from literal datatype / RDFS range checks.
enum class ValidateError : uint8_t {
  /// @brief Lexical form does not match the datatype.
  kTypeMismatch,
  /// @brief Value is outside minInclusive / maxInclusive.
  kOutOfRange,
  /// @brief Datatype is not supported by the validator.
  kUnsupportedDatatype,
  /// @brief Triple object violates an `rdfs:range` constraint.
  kRangeViolation,
};

/// @brief Result alias for validation operations.
/// @tparam T Success value type
template <typename T>
using ValidateResult = std::expected<T, ValidateError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view ValidateErrorToString(
    ValidateError error) noexcept {
  switch (error) {
    using enum ValidateError;
    case kTypeMismatch:
      return "Literal does not match datatype";
    case kOutOfRange:
      return "Literal is outside the allowed range";
    case kUnsupportedDatatype:
      return "Unsupported datatype for validation";
    case kRangeViolation:
      return "Triple violates rdfs:range";
    default:
      return "Unknown validation error";
  }
}

}  // namespace cccr::validate
