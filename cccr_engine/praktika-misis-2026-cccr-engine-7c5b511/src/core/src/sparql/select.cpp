#include <pch.hpp>

#include <cccr/sparql/select.hpp>

#include <cccr/context.hpp>
#include <cccr/log/log.hpp>
#include <cccr/rdf/graph.hpp>

#include <detail/redland_store.hpp>
#include <detail/sparql_prepare.hpp>

#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace cccr::sparql {

auto SelectFirst(std::string_view query_text, const rdf::Graph& graph,
                 Context ctx) -> SparqlResult<std::pmr::string> {
  auto* const mr = ctx.GetAllocator();
  auto& store = graph.Data()->store;
  const auto prepared =
      detail::PrepareSparqlQuery(query_text, store.Allocator());
  auto binding = store.SelectFirst(prepared, mr);
  if (!binding) [[unlikely]] {
    switch (binding.error()) {
      using enum detail::RedlandQueryError;
      case kNoResult:
        log::Error(ctx, "SPARQL SelectFirst: no result bindings");
        return std::unexpected(SparqlError::kNoResult);
      case kFailed:
        log::Error(ctx, "SPARQL SelectFirst failed");
        return std::unexpected(SparqlError::kEvalFailed);
    }
    log::Error(ctx, "SPARQL SelectFirst failed");
    return std::unexpected(SparqlError::kEvalFailed);
  }

  return std::move(*binding);
}

}  // namespace cccr::sparql
