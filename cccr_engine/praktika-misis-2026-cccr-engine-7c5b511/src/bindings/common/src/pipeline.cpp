#include <cccr/bindings/common/pipeline.hpp>

#include <cccr/csv/csv.hpp>
#include <cccr/engine/engine.hpp>
#include <cccr/json/json.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/sparql/sparql.hpp>
#include <cccr/xml/xml.hpp>

#include <exception>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace cccr::bindings {

namespace {

template <typename F>
[[nodiscard]] auto Guard(F&& fn) -> decltype(fn()) {
  try {
    return fn();
  } catch (const std::exception&) {
    return std::unexpected(BindingError::kInternal);
  } catch (...) {
    return std::unexpected(BindingError::kInternal);
  }
}

}  // namespace

auto LoadSchema(Context ctx, std::string_view schema_json,
                const std::filesystem::path& include_base)
    -> BindingResult<ast::Schema> {
  return Guard([&]() -> BindingResult<ast::Schema> {
    if (schema_json.empty()) [[unlikely]] {
      return std::unexpected(BindingError::kInvalidArg);
    }

    auto load_params = json::LoadSchemaParams{.ctx = ctx};
    load_params.include_base = include_base;
    auto schema = json::LoadSchema(schema_json, load_params);
    if (!schema) [[unlikely]] {
      return std::unexpected(BindingError::kSchemaLoad);
    }
    return std::move(*schema);
  });
}

auto LoadXmlSource(Context ctx, std::string_view xml)
    -> BindingResult<std::unique_ptr<input::Source>> {
  return Guard([&]() -> BindingResult<std::unique_ptr<input::Source>> {
    if (xml.empty()) [[unlikely]] {
      return std::unexpected(BindingError::kInvalidArg);
    }

    auto source = std::make_unique<xml::XmlSource>(ctx);
    if (const auto loaded = source->Load(xml); !loaded) [[unlikely]] {
      return std::unexpected(BindingError::kInputLoad);
    }
    return source;
  });
}

auto LoadCsvSource(Context ctx, std::string_view csv)
    -> BindingResult<std::unique_ptr<input::Source>> {
  return Guard([&]() -> BindingResult<std::unique_ptr<input::Source>> {
    if (csv.empty()) [[unlikely]] {
      return std::unexpected(BindingError::kInvalidArg);
    }

    auto source = std::make_unique<csv::CsvSource>(ctx);
    if (const auto loaded = source->Load(csv); !loaded) [[unlikely]] {
      return std::unexpected(BindingError::kInputLoad);
    }
    return source;
  });
}

auto LoadGraph(Context ctx, std::string_view text, rdf::RdfSyntax syntax)
    -> BindingResult<rdf::Graph> {
  return Guard([&]() -> BindingResult<rdf::Graph> {
    if (text.empty()) [[unlikely]] {
      return std::unexpected(BindingError::kInvalidArg);
    }

    auto graph = rdf::LoadRdf(text, syntax, ctx);
    if (!graph) [[unlikely]] {
      return std::unexpected(BindingError::kRdfLoad);
    }
    return std::move(*graph);
  });
}

auto RunEngine(Context ctx, const ast::Schema& schema, input::Source& source,
               const rdf::Graph* knowledge,
               const std::filesystem::path& include_base)
    -> BindingResult<rdf::Graph> {
  auto ref = std::reference_wrapper<input::Source>{source};
  return RunEngine(ctx, schema, std::span{&ref, 1}, knowledge, include_base);
}

auto RunEngine(Context ctx, const ast::Schema& schema,
               std::span<std::reference_wrapper<input::Source>> sources,
               const rdf::Graph* knowledge,
               const std::filesystem::path& include_base)
    -> BindingResult<rdf::Graph> {
  return Guard([&]() -> BindingResult<rdf::Graph> {
    if (sources.empty()) [[unlikely]] {
      return std::unexpected(BindingError::kInvalidArg);
    }

    auto params = engine::RunParams{.include_base = include_base, .ctx = ctx};
    if (knowledge != nullptr) {
      params.knowledge = *knowledge;
    }
    auto graph = engine::Run(schema, sources, params);
    if (!graph) [[unlikely]] {
      return std::unexpected(BindingError::kEngineFailed);
    }
    return std::move(*graph);
  });
}

auto MergeGraph(rdf::Graph& dest, const rdf::Graph& other)
    -> BindingResult<void> {
  return Guard([&]() -> BindingResult<void> {
    if (dest.Data() == other.Data()) [[unlikely]] {
      return std::unexpected(BindingError::kInvalidArg);
    }
    dest.MergeFrom(other);
    return {};
  });
}

auto SerializeRdfXml(const rdf::Graph& graph) -> BindingResult<std::string> {
  return Guard([&]() -> BindingResult<std::string> {
    const auto rdfxml = rdf::ToRdfXml(graph);
    if (rdfxml.empty()) [[unlikely]] {
      return std::unexpected(BindingError::kSerializeFailed);
    }
    return std::string{rdfxml.data(), rdfxml.size()};
  });
}

auto QueryGraph(Context ctx, const rdf::Graph& graph, std::string_view sparql,
                sparql::ResultFormat format) -> BindingResult<std::string> {
  return Guard([&]() -> BindingResult<std::string> {
    if (sparql.empty()) [[unlikely]] {
      return std::unexpected(BindingError::kInvalidArg);
    }

    auto result = sparql::QueryFormatted(sparql, graph, format, ctx);
    if (!result) [[unlikely]] {
      return std::unexpected(BindingError::kQueryFailed);
    }
    return std::string{result->data(), result->size()};
  });
}

}  // namespace cccr::bindings
