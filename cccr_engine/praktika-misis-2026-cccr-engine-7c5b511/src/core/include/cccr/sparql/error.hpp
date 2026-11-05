#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::sparql {

/// @brief Errors from SPARQL evaluation (Redland/rasqal).
enum class SparqlError : uint8_t {
  /// @brief Query text could not be parsed.
  kParseFailed,
  /// @brief Query evaluation failed.
  kEvalFailed,
  /// @brief Query returned no bindings.
  kNoResult,
};

/// @brief Result alias for SPARQL operations.
/// @tparam T Success value type
template <typename T>
using SparqlResult = std::expected<T, SparqlError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view SparqlErrorToString(
    SparqlError error) noexcept {
  switch (error) {
    using enum SparqlError;
    case kParseFailed:
      return "Failed to parse SPARQL query";
    case kEvalFailed:
      return "SPARQL evaluation failed";
    case kNoResult:
      return "SPARQL query returned no bindings";
    default:
      return "Unknown SPARQL error";
  }
}

}  // namespace cccr::sparql
