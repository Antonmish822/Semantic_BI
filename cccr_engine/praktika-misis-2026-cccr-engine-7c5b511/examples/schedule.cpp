#include "common/run_example.hpp"

#include <chrono>
#include <cstdio>
#include <format>

/// @brief Schedule (расписание) example adapted from `примеры/расписание`.
/// @details Original schema / `localFunct.xqm` / SPARQL component are kept
///          under `examples/data/schedule/`. This runnable schema uses nested
///          `context.subject`, adds a root `about`, and avoids BaseX-only
///          `Q{localFunct}` / `random:uuid()`. Optional argv[1] overrides the
///          data directory.
int main(int argc, char* argv[]) {
  const auto data = cccr::examples::ResolveDataDir(argc, argv);
  std::fprintf(stderr, "%s\n",
               std::format("schedule: data dir {}\n", data.string()).c_str());

  const auto begin = std::chrono::high_resolution_clock::now();

  const int code =
      cccr::examples::RunExample(data / "schema.json", data / "input.xml");

  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count();
  std::fprintf(stderr, "%s\n",
               std::format("\nschedule: duration {} ms\n", duration).c_str());

  return code;
}
