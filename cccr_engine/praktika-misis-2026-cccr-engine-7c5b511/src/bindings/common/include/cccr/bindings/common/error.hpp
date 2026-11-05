#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::bindings {

/// @brief Errors from the shared bindings helpers.
enum class BindingError : uint8_t {
  /// @brief A required argument was null or otherwise invalid.
  kInvalidArg,
  /// @brief Schema JSON could not be loaded.
  kSchemaLoad,
  /// @brief Input document could not be loaded.
  kInputLoad,
  /// @brief Seed or graph RDF could not be loaded.
  kRdfLoad,
  /// @brief Engine pipeline failed.
  kEngineFailed,
  /// @brief SPARQL query formatting failed.
  kQueryFailed,
  /// @brief RDF/XML serialization produced no output.
  kSerializeFailed,
  /// @brief An unexpected third-party exception was caught.
  kInternal,
};

/// @brief Result alias for bindings pipeline operations.
/// @tparam T Success value type
template <typename T>
using BindingResult = std::expected<T, BindingError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view BindingErrorToString(
    BindingError error) noexcept {
  switch (error) {
    using enum BindingError;
    case kInvalidArg:
      return "Invalid argument";
    case kSchemaLoad:
      return "Failed to load schema";
    case kInputLoad:
      return "Failed to load input";
    case kRdfLoad:
      return "Failed to load RDF data";
    case kEngineFailed:
      return "Engine run failed";
    case kQueryFailed:
      return "SPARQL query failed";
    case kSerializeFailed:
      return "RDF/XML serialization failed";
    case kInternal:
      return "Internal bindings error";
    default:
      return "Unknown bindings error";
  }
}

}  // namespace cccr::bindings
