#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace cccr::cli {

/// @brief Errors from CLI parse and command execution.
enum class CliError : uint8_t {
  /// @brief Argument parsing failed.
  kParseFailed,
  /// @brief A required path or value was missing or invalid.
  kInvalidArgs,
  /// @brief Failed to read an input file.
  kFileRead,
  /// @brief Failed to write an output file.
  kFileWrite,
  /// @brief Schema JSON could not be loaded.
  kSchemaLoad,
  /// @brief Input document could not be loaded.
  kInputLoad,
  /// @brief Seed or graph N-Triples could not be loaded.
  kRdfLoad,
  /// @brief Engine pipeline failed.
  kEngineFailed,
  /// @brief SPARQL query formatting failed.
  kQueryFailed,
  /// @brief RDF/XML serialization produced no output.
  kSerializeFailed,
};

/// @brief Result alias for CLI operations.
/// @tparam T Success value type
template <typename T>
using CliResult = std::expected<T, CliError>;

/// @brief Converts @p error to a short diagnostic string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view CliErrorToString(
    CliError error) noexcept {
  switch (error) {
    using enum CliError;
    case kParseFailed:
      return "Failed to parse command-line arguments";
    case kInvalidArgs:
      return "Invalid command-line arguments";
    case kFileRead:
      return "Failed to read file";
    case kFileWrite:
      return "Failed to write file";
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
    default:
      return "Unknown CLI error";
  }
}

}  // namespace cccr::cli
