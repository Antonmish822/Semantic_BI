#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::engine {

/// @brief Errors from running a CCCR schema over input.
enum class EngineError : uint8_t {
  /// @brief Input parse or query evaluation failed.
  kXmlFailed,
  /// @brief A `compute` / value expression failed.
  kEvalFailed,
  /// @brief Schema is incomplete or inconsistent for execution.
  kSchemaInvalid,
  /// @brief Emitted literal failed datatype / min-max / rdfs:range validation.
  kValidationFailed,
};

/// @brief Result alias for engine operations.
/// @tparam T Success value type
template <typename T>
using EngineResult = std::expected<T, EngineError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view EngineErrorToString(
    EngineError error) noexcept {
  switch (error) {
    using enum EngineError;
    case kXmlFailed:
      return "XML processing failed";
    case kEvalFailed:
      return "Expression evaluation failed";
    case kSchemaInvalid:
      return "Schema is incomplete for execution";
    case kValidationFailed:
      return "Literal or rdfs:range validation failed";
    default:
      return "Unknown engine error";
  }
}

}  // namespace cccr::engine
