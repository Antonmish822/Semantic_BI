#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::csv {

/// @brief Errors from CSV parse / projection.
enum class CsvError : uint8_t {
  /// @brief CSV text could not be parsed.
  kParseFailed,
  /// @brief Projected XML could not be loaded by the XML backend.
  kProjectFailed,
};

/// @brief Result alias for CSV operations.
/// @tparam T Success value type
template <typename T>
using CsvResult = std::expected<T, CsvError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view CsvErrorToString(
    CsvError error) noexcept {
  switch (error) {
    using enum CsvError;
    case kParseFailed:
      return "Failed to parse CSV";
    case kProjectFailed:
      return "Failed to project CSV to XML";
    default:
      return "Unknown CSV error";
  }
}

}  // namespace cccr::csv
