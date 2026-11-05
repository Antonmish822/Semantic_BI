#pragma once

#include <filesystem>
#include <string_view>

namespace cccr::detail {

/// @brief Resolves @p relative against @p include_base when relative.
/// @param relative Path from an `include` expression / property
/// @param include_base Base directory (ignored when empty or @p relative is
///                     absolute)
/// @return Resolved filesystem path
[[nodiscard]] inline std::filesystem::path ResolveIncludePath(
    std::string_view relative, const std::filesystem::path& include_base) {
  const auto path = std::filesystem::path{relative};
  if (path.is_absolute() || include_base.empty()) {
    return path;
  }
  return include_base / path;
}

}  // namespace cccr::detail
