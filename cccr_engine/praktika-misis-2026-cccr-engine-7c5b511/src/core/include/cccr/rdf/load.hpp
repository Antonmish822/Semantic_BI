#pragma once

#include <cccr/context.hpp>
#include <cccr/rdf/error.hpp>
#include <cccr/rdf/graph.hpp>
#include <cccr/rdf/syntax.hpp>

#include <concepts>
#include <ranges>
#include <string_view>
#include <utility>

namespace cccr::rdf {

/// @brief Loads RDF text in @p syntax into a graph via Redland/raptor.
/// @param text RDF document
/// @param syntax Concrete serialization syntax
/// @param ctx Shared allocator context
/// @return Graph or RdfError
[[nodiscard]] auto LoadRdf(std::string_view text, RdfSyntax syntax,
                           Context ctx = {}) -> RdfResult<Graph>;

/// @brief Loads N-Triples into a graph via Redland/raptor.
/// @param ntriples N-Triples text
/// @param ctx Shared allocator context
/// @return Graph or RdfError
[[nodiscard]] inline auto LoadNTriples(std::string_view ntriples,
                                       Context ctx = {}) -> RdfResult<Graph> {
  return LoadRdf(ntriples, RdfSyntax::kNTriples, ctx);
}

/// @brief Loads Turtle into a graph via Redland/raptor.
/// @param turtle Turtle text
/// @param ctx Shared allocator context
/// @return Graph or RdfError
[[nodiscard]] inline auto LoadTurtle(std::string_view turtle, Context ctx = {})
    -> RdfResult<Graph> {
  return LoadRdf(turtle, RdfSyntax::kTurtle, ctx);
}

/// @brief Loads RDF/XML into a graph via Redland/raptor.
/// @param rdfxml RDF/XML text
/// @param ctx Shared allocator context
/// @return Graph or RdfError
[[nodiscard]] inline auto LoadRdfXml(std::string_view rdfxml, Context ctx = {})
    -> RdfResult<Graph> {
  return LoadRdf(rdfxml, RdfSyntax::kRdfXml, ctx);
}

/// @brief Loads multiple RDF documents of the same @p syntax and merges them.
/// @tparam R Input range whose elements convert to `std::string_view`
/// @param documents RDF document texts
/// @param syntax Concrete serialization syntax for every document
/// @param ctx Shared allocator context
/// @return Merged graph or the first RdfError from a failed document
/// @note Accepts ranges of `std::string`, `std::pmr::string`,
///       `std::string_view`, and similar owning/viewing string types.
template <std::ranges::input_range R>
  requires std::convertible_to<std::ranges::range_reference_t<R>,
                               std::string_view>
[[nodiscard]] auto LoadRdf(R&& documents, RdfSyntax syntax, Context ctx = {})
    -> RdfResult<Graph> {
  auto out = Graph::FromContext(ctx);
  for (auto&& doc : std::forward<R>(documents)) {
    auto part = LoadRdf(std::string_view{doc}, syntax, ctx);
    if (!part) [[unlikely]] {
      return std::unexpected(part.error());
    }
    out.MergeFrom(*part);
  }
  return out;
}

/// @brief Loads multiple N-Triples documents and merges them into one graph.
/// @tparam R Input range whose elements convert to `std::string_view`
/// @param documents N-Triples document texts
/// @param ctx Shared allocator context
/// @return Merged graph or the first RdfError from a failed document
template <std::ranges::input_range R>
  requires std::convertible_to<std::ranges::range_reference_t<R>,
                               std::string_view>
[[nodiscard]] auto LoadNTriples(R&& documents, Context ctx = {})
    -> RdfResult<Graph> {
  return LoadRdf(std::forward<R>(documents), RdfSyntax::kNTriples, ctx);
}

}  // namespace cccr::rdf
