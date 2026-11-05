#pragma once

#include <cccr/ast/types.hpp>
#include <cccr/context.hpp>
#include <cccr/engine/error.hpp>
#include <cccr/input/source.hpp>
#include <cccr/rdf/graph.hpp>

#include <concepts>
#include <filesystem>
#include <ranges>
#include <utility>

namespace cccr::engine {

/// @brief Optional parameters for @ref Run.
struct RunParams {
  /// @brief Seed knowledge graph for in-pipeline SPARQL.
  rdf::Graph knowledge;

  /// @brief Directory for resolving relative `include` paths.
  std::filesystem::path include_base = {};

  /// @brief Shared allocator context.
  Context ctx;
};

/// @brief Applies @p schema to @p source and returns an RDF graph.
/// @param schema Parsed CCCR schema
/// @param source Loaded input document (e.g. @ref xml::XmlSource)
/// @param params Optional knowledge graph, include base, and allocator
/// @return Output RDF graph or EngineError
[[nodiscard]] auto Run(const ast::Schema& schema, input::Source& source,
                       const RunParams& params = {})
    -> EngineResult<rdf::Graph>;

/// @brief Runs @p schema over each source in @p sources and merges outputs.
/// @tparam R Input range whose elements convert to `input::Source&`
/// @param schema Parsed CCCR schema
/// @param sources Loaded input documents
/// @param params Optional knowledge graph, include base, and allocator
/// @return Merged output RDF graph or the first EngineError
/// @note Accepts ranges of concrete `Source` subclasses by value (element
///       access yields `T&` → `Source&`), `Source&`, or
///       `std::reference_wrapper<Source>`. Does not accept ranges of prvalues
///       only (cannot bind a temporary to non-const `Source&`). `Source` itself
///       is abstract and cannot be stored by value.
template <std::ranges::input_range R>
  requires std::convertible_to<std::ranges::range_reference_t<R>,
                               input::Source&>
[[nodiscard]] auto Run(const ast::Schema& schema, R&& sources,
                       const RunParams& params = {})
    -> EngineResult<rdf::Graph> {
  auto out = rdf::Graph::FromContext(params.ctx);
  for (auto&& source : std::forward<R>(sources)) {
    input::Source& ref = source;
    auto part = Run(schema, ref, params);
    if (!part) [[unlikely]] {
      return std::unexpected(part.error());
    }
    out.MergeFrom(*part);
  }

  return out;
}

}  // namespace cccr::engine
