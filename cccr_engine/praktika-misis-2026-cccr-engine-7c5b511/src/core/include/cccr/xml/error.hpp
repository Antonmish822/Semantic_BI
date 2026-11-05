#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::xml {

/// @brief Errors from XML parse / XQuery evaluation via Saxon.
enum class XmlError : uint8_t {
  /// @brief XML document could not be parsed.
  kParseFailed,
  /// @brief XQuery evaluation failed.
  kEvalFailed,
};

/// @brief Result alias for XML operations.
/// @tparam T Success value type
template <typename T>
using XmlResult = std::expected<T, XmlError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view XmlErrorToString(
    XmlError error) noexcept {
  switch (error) {
    using enum XmlError;
    case kParseFailed:
      return "Failed to parse XML";
    case kEvalFailed:
      return "XQuery evaluation failed";
    default:
      return "Unknown XML error";
  }
}

}  // namespace cccr::xml
