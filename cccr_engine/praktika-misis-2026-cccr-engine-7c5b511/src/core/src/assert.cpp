#include <cccr/assert.hpp>

#include <cccr/utils/filesystem.hpp>

#include <cstdio>
#include <format>
#include <iterator>
#include <source_location>
#include <string>
#include <string_view>

namespace cccr {

namespace details {

void DefaultAssertionHandler(std::string_view condition,
                             const std::source_location& loc,
                             std::string_view message) noexcept {
  try {
    const std::string formatted =
        FormatAssertionMessage(condition, loc, message);

    std::fprintf(stderr, "%.*s\n", static_cast<int>(formatted.size()),
                 formatted.data());
    std::fflush(stderr);

  } catch (...) {
    [[maybe_unused]] const int write_result = std::fputs(
        "Assertion failed (error while formatting or logging)\n", stderr);
  }
}

}  // namespace details

std::string FormatAssertionMessage(std::string_view condition,
                                   const std::source_location& loc,
                                   std::string_view message) {
  std::string result;
  result.reserve(256);

  if (!message.empty()) {
    std::format_to(std::back_inserter(result), "Assertion failed: {} | {}",
                   condition, message);
  } else {
    std::format_to(std::back_inserter(result), "Assertion failed: {}",
                   condition);
  }

  const auto filename = utils::GetFileName(loc.file_name());
  std::format_to(std::back_inserter(result), " [{}:{}]", filename, loc.line());

  return result;
}

}  // namespace cccr
