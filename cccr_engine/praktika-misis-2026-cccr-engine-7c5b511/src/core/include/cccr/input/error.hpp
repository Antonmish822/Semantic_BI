#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::input {

/// @brief Errors from format-agnostic input evaluation.
enum class InputError : uint8_t {
  /// @brief Input document could not be parsed.
  kParseFailed,
  /// @brief Backend query evaluation failed.
  kEvalFailed,
};

/// @brief Result alias for input operations.
/// @tparam T Success value type
template <typename T>
using InputResult = std::expected<T, InputError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view InputErrorToString(
    InputError error) noexcept {
  switch (error) {
    using enum InputError;
    case kParseFailed:
      return "Failed to parse input";
    case kEvalFailed:
      return "Input query evaluation failed";
    default:
      return "Unknown input error";
  }
}

}  // namespace cccr::input
