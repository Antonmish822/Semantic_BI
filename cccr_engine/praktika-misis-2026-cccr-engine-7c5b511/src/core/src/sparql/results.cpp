#include <pch.hpp>

#include <cccr/sparql/results.hpp>

#include <cccr/log/log.hpp>
#include <cccr/rdf/graph.hpp>

#include <detail/redland_store.hpp>
#include <detail/sparql_prepare.hpp>

#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace cccr::sparql {
namespace {

[[nodiscard]] constexpr std::string_view FormatName(
    ResultFormat format) noexcept {
  switch (format) {
    using enum ResultFormat;
    case kJson:
      return "json";
    case kXml:
      return "xml";
    case kHtml:
      return "html";
  }
  return "json";
}

}  // namespace

auto QueryFormatted(std::string_view query, const rdf::Graph& graph,
                    ResultFormat format, Context ctx)
    -> SparqlResult<std::pmr::string> {
  auto* const mr = ctx.GetAllocator();
  auto& store = graph.Data()->store;
  const auto prepared = detail::PrepareSparqlQuery(query, store.Allocator());
  auto formatted = store.QueryFormatted(prepared, FormatName(format), mr);
  if (!formatted) [[unlikely]] {
    switch (formatted.error()) {
      using enum detail::RedlandQueryError;
      case kNoResult:
        log::Error(ctx, "SPARQL QueryFormatted: no result");
        return std::unexpected(SparqlError::kNoResult);
      case kFailed:
        log::Error(ctx, "SPARQL QueryFormatted failed (format={})",
                   FormatName(format));
        return std::unexpected(SparqlError::kEvalFailed);
    }
    log::Error(ctx, "SPARQL QueryFormatted failed (format={})",
               FormatName(format));
    return std::unexpected(SparqlError::kEvalFailed);
  }

  return std::move(*formatted);
}

}  // namespace cccr::sparql
