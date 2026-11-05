#pragma once

#include <detail/text.hpp>

#include <cstddef>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>

namespace cccr::detail {

/// @brief Parses an IRI reference `<...>` at @p idx.
/// @param text Source text
/// @param idx Cursor; advanced past the closing `>` on success
/// @param allocator PMR resource for the returned string
/// @return IRI contents (without brackets), or `std::nullopt` on failure
[[nodiscard]] inline auto ParseIriRef(
    std::string_view text, size_t& idx,
    std::pmr::memory_resource* allocator = std::pmr::get_default_resource())
    -> std::optional<std::pmr::string> {
  SkipWs(text, idx);
  if (idx >= text.size() || text[idx] != '<') [[unlikely]] {
    return std::nullopt;
  }

  ++idx;
  const size_t start = idx;
  while (idx < text.size() && text[idx] != '>') {
    ++idx;
  }

  if (idx >= text.size()) [[unlikely]] {
    return std::nullopt;
  }

  auto iri = std::pmr::string{text.substr(start, idx - start), allocator};
  ++idx;
  return iri;
}

/// @brief Joins a namespace IRI and local name, inserting `#` when needed.
/// @param ns Namespace base (may already end with `/` or `#`)
/// @param local Local name
/// @param allocator PMR resource for the returned string
/// @return Absolute IRI string
[[nodiscard]] inline std::pmr::string JoinNsLocal(
    std::string_view ns, std::string_view local,
    std::pmr::memory_resource* allocator = std::pmr::get_default_resource()) {
  auto out = std::pmr::string{ns, allocator};
  if (!out.empty() && out.back() != '/' && out.back() != '#') {
    out.push_back('#');
  }
  out.append(local);
  return out;
}

}  // namespace cccr::detail
