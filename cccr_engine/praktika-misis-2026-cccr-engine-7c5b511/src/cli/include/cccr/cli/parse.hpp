#pragma once

#include <cccr/cli/error.hpp>
#include <cccr/cli/options.hpp>

namespace cccr::cli {

/// @brief Parses @p argc / @p argv into a @ref ParsedCommand.
/// @details Uses argparse subcommands `run` and `query`. Help and version
///          call `std::exit(0)` via argparse defaults.
/// @param argc Argument count
/// @param argv Argument vector
/// @return Parsed command or CliError
[[nodiscard]] auto ParseArgs(int argc, char* argv[])
    -> CliResult<ParsedCommand>;

}  // namespace cccr::cli
