#pragma once

#include <cccr/context.hpp>
#include <cccr/csv/csv.hpp>
#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/utils/filesystem.hpp>
#include <cccr/xml/xml.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <format>
#include <memory_resource>
#include <string>

namespace cccr::examples {

namespace detail {

[[nodiscard]] constexpr bool EqualsIgnoreCase(std::string_view left,
                                              std::string_view right) noexcept {
  if (left.size() != right.size()) {
    return false;
  }

  return std::ranges::equal(left, right, [](char lhs, char rhs) {
    return std::tolower(static_cast<unsigned char>(lhs)) ==
           std::tolower(static_cast<unsigned char>(rhs));
  });
}

[[nodiscard]] inline bool IsCsvPath(
    const std::filesystem::path& path) noexcept {
  return EqualsIgnoreCase(path.extension().string(), ".csv");
}

}  // namespace detail

/// @brief Loads schema + XML/CSV input (+ optional N-Triples seed), runs the
///        engine, and prints RDF/XML.
/// @details Input kind is chosen from @p input_path extension: `.csv` →
///          @ref csv::CsvSource, otherwise @ref xml::XmlSource.
/// @param schema_path Path to CCCR JSON schema
/// @param input_path Path to XML or CSV input
/// @param seed_path Optional N-Triples seed for SPARQL (`compute.sparql`);
///                  empty skips
/// @return Process exit code (`0` on success)
[[nodiscard]] inline int RunExample(
    const std::filesystem::path& schema_path,
    const std::filesystem::path& input_path,
    const std::filesystem::path& seed_path = {}) {
  constexpr size_t kPoolSize = 16 * 1024;  // 16 KB
  std::pmr::monotonic_buffer_resource arena(kPoolSize);
  Context ctx{.allocator = &arena};
  const auto include_base = schema_path.parent_path();

  const auto schema_text = utils::ReadFileToString(schema_path, ctx);
  if (!schema_text) {
    std::fprintf(
        stderr, "%s\n",
        std::format("Failed to read schema {}: {}", schema_path.string(),
                    utils::FileErrorToString(schema_text.error()))
            .c_str());
    return 1;
  }

  const auto input_text = utils::ReadFileToString(input_path, ctx);
  if (!input_text) {
    std::fprintf(stderr, "%s\n",
                 std::format("Failed to read input {}: {}", input_path.string(),
                             utils::FileErrorToString(input_text.error()))
                     .c_str());
    return 1;
  }

  auto load_params = json::LoadSchemaParams{.ctx = ctx};
  load_params.include_base = include_base;
  const auto schema = json::LoadSchema(*schema_text, load_params);
  if (!schema) {
    std::fprintf(stderr, "%s\n",
                 std::format("Failed to load schema: {}",
                             json::JsonErrorToString(schema.error()))
                     .c_str());
    return 1;
  }

  auto params =
      engine::RunParams{.knowledge = {}, .include_base = {}, .ctx = ctx};
  params.include_base = include_base;
  if (!seed_path.empty()) {
    const auto seed_text = utils::ReadFileToString(seed_path, ctx);
    if (!seed_text) {
      std::fprintf(stderr, "%s\n",
                   std::format("Failed to read seed {}: {}", seed_path.string(),
                               utils::FileErrorToString(seed_text.error()))
                       .c_str());
      return 1;
    }

    auto knowledge = rdf::LoadNTriples(*seed_text, ctx);
    if (!knowledge) {
      std::fprintf(stderr, "%s\n",
                   std::format("Failed to parse seed N-Triples: {}",
                               rdf::RdfErrorToString(knowledge.error()))
                       .c_str());
      return 1;
    }

    params.knowledge = std::move(*knowledge);
  }

  const auto run = [&](input::Source& source) -> int {
    const auto graph = engine::Run(*schema, source, params);
    if (!graph) {
      std::fprintf(stderr, "%s\n",
                   std::format("engine::Run failed: {}",
                               engine::EngineErrorToString(graph.error()))
                       .c_str());
      return 1;
    }
    const auto rdfxml = rdf::ToRdfXml(*graph);
    std::fwrite(rdfxml.data(), 1, rdfxml.size(), stdout);
    return 0;
  };

  if (detail::IsCsvPath(input_path)) {
    csv::CsvSource source;
    if (const auto loaded = source.Load(*input_text); !loaded) {
      std::fprintf(stderr, "%s\n",
                   std::format("Failed to load CSV: {}",
                               csv::CsvErrorToString(loaded.error()))
                       .c_str());
      return 1;
    }
    return run(source);
  }

  xml::XmlSource source;
  if (const auto loaded = source.Load(*input_text); !loaded) {
    std::fprintf(stderr, "%s\n",
                 std::format("Failed to load XML: {}",
                             xml::XmlErrorToString(loaded.error()))
                     .c_str());
    return 1;
  }
  return run(source);
}

/// @brief Resolves the example data directory.
/// @param argc Argument count
/// @param argv Argument vector; optional `argv[1]` overrides the data directory
/// @return Absolute path to the example data directory
[[nodiscard]] inline std::filesystem::path ResolveDataDir(int argc,
                                                          char* argv[]) {
  if (argc > 1) {
    return std::filesystem::path{argv[1]};
  }
#ifdef CCCR_EXAMPLE_DATA_DIR
  return std::filesystem::path{CCCR_EXAMPLE_DATA_DIR};
#else
  return std::filesystem::current_path();
#endif
}

}  // namespace cccr::examples
