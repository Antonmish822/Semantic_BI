#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::json {

/// @brief Errors from loading a CCCR JSON schema.
enum class JsonError : uint8_t {
  /// @brief JSON text could not be parsed.
  kParseFailed,
  /// @brief JSON is not a valid CCCR schema subset.
  kInvalidSchema,
  /// @brief An included schema fragment could not be loaded.
  kIncludeFailed,
};

/// @brief Result alias for JSON operations.
/// @tparam T Success value type
template <typename T>
using JsonResult = std::expected<T, JsonError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view JsonErrorToString(
    JsonError error) noexcept {
  switch (error) {
    using enum JsonError;
    case kParseFailed:
      return "Failed to parse JSON";
    case kInvalidSchema:
      return "JSON is not a valid CCCR schema subset";
    case kIncludeFailed:
      return "Failed to load an included schema fragment";
    default:
      return "Unknown JSON error";
  }
}

}  // namespace cccr::json
