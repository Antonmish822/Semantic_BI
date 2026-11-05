#pragma once

#include <cctype>
#include <cstddef>
#include <string_view>

namespace cccr::detail {

/// @brief Returns whether @p ch is ASCII whitespace.
/// @param ch The character to check
/// @return true if @p ch is whitespace, false otherwise
[[nodiscard]] constexpr bool IsSpace(char ch) noexcept {
  return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

/// @brief Advances @p idx past consecutive whitespace in @p text.
/// @param text The text to scan
/// @param idx The index to advance
/// @returns The index after the last whitespace character
constexpr void SkipWs(std::string_view text, size_t& idx) noexcept {
  while (idx < text.size() && IsSpace(text[idx])) {
    ++idx;
  }
}

}  // namespace cccr::detail
