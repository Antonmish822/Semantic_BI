#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::eval {

/// @brief Errors from evaluating a `compute` / value expression.
enum class EvalError : uint8_t {
  /// @brief XQuery evaluation failed.
  kXQueryFailed,
  /// @brief SPARQL evaluation failed.
  kSparqlFailed,
  /// @brief Include path could not be read.
  kIncludeFailed,
  /// @brief Value expression has neither literal nor compute.
  kMissingValue,
  /// @brief Evaluator kind or form is not supported.
  kUnsupported,
};

/// @brief Result alias for expression evaluation.
/// @tparam T Success value type
template <typename T>
using EvalResult = std::expected<T, EvalError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view EvalErrorToString(
    EvalError error) noexcept {
  switch (error) {
    using enum EvalError;
    case kXQueryFailed:
      return "XQuery evaluation failed";
    case kSparqlFailed:
      return "SPARQL evaluation failed";
    case kIncludeFailed:
      return "Include file could not be read";
    case kMissingValue:
      return "Value expression is empty";
    case kUnsupported:
      return "Unsupported evaluator";
    default:
      return "Unknown eval error";
  }
}

}  // namespace cccr::eval
