#include <pch.hpp>

#include <cccr/rdf/load.hpp>

#include <cccr/context.hpp>
#include <cccr/log/log.hpp>
#include <cccr/rdf/graph.hpp>
#include <cccr/rdf/syntax.hpp>

#include <detail/redland_store.hpp>

#include <expected>
#include <utility>

namespace cccr::rdf {

auto LoadRdf(std::string_view text, RdfSyntax syntax, Context ctx)
    -> RdfResult<Graph> {
  auto graph = Graph::FromContext(ctx);
  auto& data = *graph.Data();
  if (!data.store.Parse(text, RdfSyntaxToRaptorName(syntax))) [[unlikely]] {
    log::Error(ctx, "{} parse failed", RdfSyntaxToString(syntax));
    return std::unexpected(RdfError::kParseFailed);
  }

  auto triples = data.store.MaterializeTriples();
  if (!triples) [[unlikely]] {
    log::Error(ctx, "{} parse failed: could not materialize triples",
               RdfSyntaxToString(syntax));
    return std::unexpected(RdfError::kParseFailed);
  }

  data.triples = std::move(*triples);
  return graph;
}

}  // namespace cccr::rdf
