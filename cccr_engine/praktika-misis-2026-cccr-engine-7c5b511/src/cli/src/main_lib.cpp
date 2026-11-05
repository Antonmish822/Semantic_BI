#include <pch.hpp>

#include <cccr/cli/cli.hpp>

#include <cccr/log/log.hpp>

#include <concepts>
#include <type_traits>
#include <variant>

namespace cccr::cli {

auto Main(int argc, char* argv[]) -> int {
  auto parsed = ParseArgs(argc, argv);
  if (!parsed) {
    log::Error(CliErrorToString(parsed.error()));
    return 1;
  }

  const auto result = std::visit(
      [](const auto& options) -> CliResult<void> {
        using T = std::decay_t<decltype(options)>;
        if constexpr (std::same_as<T, RunOptions>) {
          return ExecuteRun(options);
        } else {
          return ExecuteQuery(options);
        }
      },
      *parsed);

  if (!result) {
    log::Error(CliErrorToString(result.error()));
    return 1;
  }
  return 0;
}

}  // namespace cccr::cli
