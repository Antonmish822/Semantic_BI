#pragma once

#include <cstdint>
#include <string_view>

namespace cccr::log {

/// @brief Severity levels for @ref cccr::log free functions.
enum class Level : uint8_t {
  kDebug,
  kTrace,
  kInfo,
  kWarn,
  kError,
  kCritical,
};

/// @brief Converts @p level to a short lowercase label for log prefixes.
/// @param level The log level
/// @return Human-readable level name
[[nodiscard]] constexpr std::string_view LevelToString(Level level) noexcept {
  switch (level) {
    using enum Level;
    case kDebug:
      return "debug";
    case kTrace:
      return "trace";
    case kInfo:
      return "info";
    case kWarn:
      return "warn";
    case kError:
      return "error";
    case kCritical:
      return "critical";
    default:
      return "unknown";
  }
}

}  // namespace cccr::log
