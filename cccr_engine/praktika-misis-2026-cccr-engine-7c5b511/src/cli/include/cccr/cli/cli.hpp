#pragma once

#include <cccr/cli/error.hpp>
#include <cccr/cli/parse.hpp>
#include <cccr/cli/run.hpp>

namespace cccr::cli {

/// @brief Parses arguments, dispatches `run` / `query`, returns process status.
/// @param argc Argument count
/// @param argv Argument vector
/// @return `0` on success, `1` on failure
[[nodiscard]] int Main(int argc, char* argv[]);

}  // namespace cccr::cli
