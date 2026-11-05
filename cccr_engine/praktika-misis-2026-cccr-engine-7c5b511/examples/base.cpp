#include "common/run_example.hpp"

#include <chrono>
#include <cstdio>
#include <format>

/// @brief End-to-end example from `пример.md` (XML registry → RDF/XML).
/// @details Optional argv[1] overrides the data directory (defaults to
///          `CCCR_EXAMPLE_DATA_DIR`).
int main(int argc, char* argv[]) {
  const auto data = cccr::examples::ResolveDataDir(argc, argv);
  std::fprintf(stderr, "%s\n",
               std::format("base: data dir {}\n", data.string()).c_str());

  const auto begin = std::chrono::high_resolution_clock::now();

  const int code = cccr::examples::RunExample(
      data / "schema.json", data / "input.xml", data / "seed.nt");

  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();
  std::fprintf(stderr, "%s\n",
               std::format("\nbase: duration {} ms\n", duration).c_str());

  return code;
}
