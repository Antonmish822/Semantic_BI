#pragma once

#include <cccr/cli/error.hpp>

#include <filesystem>
#include <string_view>

namespace cccr::cli {

/// @brief Writes @p text to @p path, or to stdout when @p path is empty.
/// @param path Destination file path; empty means stdout
/// @param text Bytes to write
/// @return Empty success or CliError
[[nodiscard]] auto WriteOutput(const std::filesystem::path& path,
                               std::string_view text) -> CliResult<void>;

}  // namespace cccr::cli
