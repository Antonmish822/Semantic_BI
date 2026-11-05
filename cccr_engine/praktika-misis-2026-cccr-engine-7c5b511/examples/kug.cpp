#include "common/run_example.hpp"

#include <chrono>
#include <cstdio>
#include <format>

/// @brief Academic calendar (КУГ) example adapted from `примеры/куг`.
/// @details Loads CSV (projected to `<registry>/<row>/<cell>`), not the
///          original Excel/XML shape. Original BaseX-oriented schema is kept as
///          `examples/data/kug/original-schema.json`. Optional argv[1]
///          overrides the data directory.
int main(int argc, char* argv[]) {
  const auto data = cccr::examples::ResolveDataDir(argc, argv);
  std::fprintf(stderr, "%s\n",
               std::format("kug: data dir {}\n", data.string()).c_str());

  const auto begin = std::chrono::high_resolution_clock::now();

  const int code =
      cccr::examples::RunExample(data / "schema.json", data / "input.csv");

  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();
  std::fprintf(stderr, "%s\n",
               std::format("\nkug: duration {} ms\n", duration).c_str());

  return code;
}
