#include <pch.hpp>

#include <cccr/cli/run.hpp>

#include <cccr/cli/output.hpp>
#include <cccr/context.hpp>
#include <cccr/csv/csv.hpp>
#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/log/log.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/sparql/sparql.hpp>
#include <cccr/utils/filesystem.hpp>
#include <cccr/xml/xml.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <memory>
#include <memory_resource>
#include <string_view>
#include <utility>

namespace cccr::cli {
namespace {

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

[[nodiscard]] bool IsCsvPath(const std::filesystem::path& path) noexcept {
  return EqualsIgnoreCase(path.extension().string(), ".csv");
}

[[nodiscard]] bool IsCsvInput(const std::filesystem::path& path,
                              InputFormat format) noexcept {
  switch (format) {
    using enum InputFormat;
    case kCsv:
      return true;
    case kXml:
      return false;
    case kAuto:
    default:
      return IsCsvPath(path);
  }
}

[[nodiscard]] auto ResolveGraphSyntax(const std::filesystem::path& path,
                                      GraphFormat format)
    -> CliResult<rdf::RdfSyntax> {
  switch (format) {
    using enum GraphFormat;
    case kNTriples:
      return rdf::RdfSyntax::kNTriples;
    case kTurtle:
      return rdf::RdfSyntax::kTurtle;
    case kRdfXml:
      return rdf::RdfSyntax::kRdfXml;
    case kAuto:
    default:
      break;
  }

  const auto ext = path.extension().string();
  if (EqualsIgnoreCase(ext, ".ttl") || EqualsIgnoreCase(ext, ".turtle")) {
    return rdf::RdfSyntax::kTurtle;
  }

  if (EqualsIgnoreCase(ext, ".rdf") || EqualsIgnoreCase(ext, ".owl") ||
      EqualsIgnoreCase(ext, ".rdfxml")) {
    return rdf::RdfSyntax::kRdfXml;
  }

  if (EqualsIgnoreCase(ext, ".nt") || EqualsIgnoreCase(ext, ".ntriples")) {
    return rdf::RdfSyntax::kNTriples;
  }

  log::Error("Cannot infer RDF syntax for {}; use --graph-format",
             path.string());
  return std::unexpected(CliError::kInvalidArgs);
}

[[nodiscard]] auto ResolveQueryText(std::string_view query, Context ctx)
    -> CliResult<std::pmr::string> {
  const std::filesystem::path as_path{std::string{query}};
  if (!as_path.empty() && std::filesystem::is_regular_file(as_path)) {
    auto text = utils::ReadFileToString(as_path, ctx);
    if (!text) [[unlikely]] {
      log::Error(ctx, "Failed to read query {}: {}", as_path.string(),
                 utils::FileErrorToString(text.error()));
      return std::unexpected(CliError::kFileRead);
    }
    return std::move(*text);
  }

  return std::pmr::string{query, ctx.GetAllocator()};
}

}  // namespace

auto ExecuteRun(const RunOptions& options) -> CliResult<void> {
  if (options.schema.empty() || options.inputs.empty()) [[unlikely]] {
    return std::unexpected(CliError::kInvalidArgs);
  }

  constexpr size_t kPoolSize = 1024 * 1024;
  std::pmr::monotonic_buffer_resource arena(kPoolSize);
  Context ctx{.allocator = &arena};
  auto* const mr = ctx.GetAllocator();
  const auto include_base = options.schema.parent_path();

  const auto schema_text = utils::ReadFileToString(options.schema, ctx);
  if (!schema_text) [[unlikely]] {
    log::Error(ctx, "Failed to read schema {}: {}", options.schema.string(),
               utils::FileErrorToString(schema_text.error()));
    return std::unexpected(CliError::kFileRead);
  }

  auto load_params = json::LoadSchemaParams{.ctx = ctx};
  load_params.include_base = include_base;
  const auto schema = json::LoadSchema(*schema_text, load_params);
  if (!schema) [[unlikely]] {
    log::Error(ctx, "Failed to load schema: {}",
               json::JsonErrorToString(schema.error()));
    return std::unexpected(CliError::kSchemaLoad);
  }

  auto params = engine::RunParams{
      .knowledge = {}, .include_base = include_base, .ctx = ctx};

  if (!options.seeds.empty()) {
    std::pmr::vector<std::pmr::string> seed_texts{mr};
    seed_texts.reserve(options.seeds.size());
    std::pmr::vector<std::string_view> seed_views{mr};
    seed_views.reserve(options.seeds.size());

    for (const auto& seed_path : options.seeds) {
      auto seed_text = utils::ReadFileToString(seed_path, ctx);
      if (!seed_text) [[unlikely]] {
        log::Error(ctx, "Failed to read seed {}: {}", seed_path.string(),
                   utils::FileErrorToString(seed_text.error()));
        return std::unexpected(CliError::kFileRead);
      }
      seed_texts.push_back(std::move(*seed_text));
      seed_views.emplace_back(seed_texts.back());
    }

    auto knowledge = rdf::LoadNTriples(seed_views, ctx);
    if (!knowledge) [[unlikely]] {
      log::Error(ctx, "Failed to parse seed N-Triples: {}",
                 rdf::RdfErrorToString(knowledge.error()));
      return std::unexpected(CliError::kRdfLoad);
    }
    params.knowledge = std::move(*knowledge);
  }

  std::pmr::vector<std::unique_ptr<input::Source>> owned{mr};
  owned.reserve(options.inputs.size());
  std::pmr::vector<std::reference_wrapper<input::Source>> refs{mr};
  refs.reserve(options.inputs.size());

  for (const auto& input_path : options.inputs) {
    auto input_text = utils::ReadFileToString(input_path, ctx);
    if (!input_text) [[unlikely]] {
      log::Error(ctx, "Failed to read input {}: {}", input_path.string(),
                 utils::FileErrorToString(input_text.error()));
      return std::unexpected(CliError::kFileRead);
    }

    if (IsCsvInput(input_path, options.input_format)) {
      auto source = std::make_unique<csv::CsvSource>(ctx);
      if (const auto loaded = source->Load(*input_text); !loaded) [[unlikely]] {
        log::Error(ctx, "Failed to load CSV {}: {}", input_path.string(),
                   csv::CsvErrorToString(loaded.error()));
        return std::unexpected(CliError::kInputLoad);
      }
      refs.emplace_back(*source);
      owned.push_back(std::move(source));
    } else {
      auto source = std::make_unique<xml::XmlSource>(ctx);
      if (const auto loaded = source->Load(*input_text); !loaded) [[unlikely]] {
        log::Error(ctx, "Failed to load XML {}: {}", input_path.string(),
                   xml::XmlErrorToString(loaded.error()));
        return std::unexpected(CliError::kInputLoad);
      }
      refs.emplace_back(*source);
      owned.push_back(std::move(source));
    }
  }

  const auto graph = engine::Run(*schema, refs, params);
  if (!graph) [[unlikely]] {
    log::Error(ctx, "engine::Run failed: {}",
               engine::EngineErrorToString(graph.error()));
    return std::unexpected(CliError::kEngineFailed);
  }

  const auto rdfxml = rdf::ToRdfXml(*graph);
  if (rdfxml.empty()) [[unlikely]] {
    return std::unexpected(CliError::kSerializeFailed);
  }

  return WriteOutput(options.output, rdfxml);
}

auto ExecuteQuery(const QueryOptions& options) -> CliResult<void> {
  if (options.graphs.empty() || options.query.empty()) [[unlikely]] {
    return std::unexpected(CliError::kInvalidArgs);
  }

  constexpr size_t kPoolSize = 1024 * 1024;
  std::pmr::monotonic_buffer_resource arena(kPoolSize);
  Context ctx{.allocator = &arena};

  auto graph = rdf::Graph::FromContext(ctx);
  for (const auto& graph_path : options.graphs) {
    auto text = utils::ReadFileToString(graph_path, ctx);
    if (!text) [[unlikely]] {
      log::Error(ctx, "Failed to read graph {}: {}", graph_path.string(),
                 utils::FileErrorToString(text.error()));
      return std::unexpected(CliError::kFileRead);
    }

    const auto syntax = ResolveGraphSyntax(graph_path, options.graph_format);
    if (!syntax) [[unlikely]] {
      return std::unexpected(syntax.error());
    }

    auto part = rdf::LoadRdf(*text, *syntax, ctx);
    if (!part) [[unlikely]] {
      log::Error(ctx, "Failed to parse {} as {}: {}", graph_path.string(),
                 rdf::RdfSyntaxToString(*syntax),
                 rdf::RdfErrorToString(part.error()));
      return std::unexpected(CliError::kRdfLoad);
    }
    graph.MergeFrom(*part);
  }

  auto query_text = ResolveQueryText(options.query, ctx);
  if (!query_text) [[unlikely]] {
    return std::unexpected(query_text.error());
  }

  auto result = sparql::QueryFormatted(*query_text, graph, options.format, ctx);
  if (!result) [[unlikely]] {
    log::Error(ctx, "SPARQL query failed: {}",
               sparql::SparqlErrorToString(result.error()));
    return std::unexpected(CliError::kQueryFailed);
  }

  return WriteOutput(options.output, *result);
}

}  // namespace cccr::cli
