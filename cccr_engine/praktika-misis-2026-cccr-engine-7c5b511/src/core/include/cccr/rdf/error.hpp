#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::rdf {

/// @brief Errors from RDF load / serialize helpers.
enum class RdfError : uint8_t {
  /// @brief Seed RDF text could not be parsed.
  kParseFailed,
};

/// @brief Result alias for RDF operations.
/// @tparam T Success value type
template <typename T>
using RdfResult = std::expected<T, RdfError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view RdfErrorToString(
    RdfError error) noexcept {
  switch (error) {
    using enum RdfError;
    case kParseFailed:
      return "Failed to parse RDF seed data";
    default:
      return "Unknown RDF error";
  }
}

}  // namespace cccr::rdf
