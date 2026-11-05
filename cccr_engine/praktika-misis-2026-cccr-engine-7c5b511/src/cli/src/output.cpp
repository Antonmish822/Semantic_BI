#include <pch.hpp>

#include <cccr/cli/output.hpp>

#include <cccr/log/log.hpp>
#include <cccr/utils/filesystem.hpp>

#include <cstdio>
#include <string_view>

namespace cccr::cli {

auto WriteOutput(const std::filesystem::path& path, std::string_view text)
    -> CliResult<void> {
  if (path.empty()) {
    // Program payload (RDF/XML / query result), not a diagnostic.
    std::fwrite(text.data(), 1, text.size(), stdout);
    return {};
  }

  const auto written = utils::WriteStringToFile(path, text);
  if (!written) {
    log::Error("Failed to write {}: {}", path.string(),
               utils::FileErrorToString(written.error()));
    return std::unexpected(CliError::kFileWrite);
  }
  return {};
}

}  // namespace cccr::cli
