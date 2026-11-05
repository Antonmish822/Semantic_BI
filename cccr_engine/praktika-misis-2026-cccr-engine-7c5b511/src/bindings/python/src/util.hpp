#pragma once

#include <cccr/bindings/common/error.hpp>
#include <cccr/rdf/syntax.hpp>
#include <cccr/sparql/results.hpp>

#include <expected>
#include <string_view>

namespace cccr::bindings::python {

/// @brief Parses a graph format name into @ref rdf::RdfSyntax.
/// @param format `"ntriples"`, `"turtle"`, or `"rdfxml"`
/// @return Syntax, or BindingError::kInvalidArg
[[nodiscard]] constexpr auto ParseGraphFormat(std::string_view format) noexcept
    -> BindingResult<rdf::RdfSyntax> {
  if (format == "ntriples") [[likely]] {
    return rdf::RdfSyntax::kNTriples;
  }

  if (format == "turtle") {
    return rdf::RdfSyntax::kTurtle;
  }

  if (format == "rdfxml") {
    return rdf::RdfSyntax::kRdfXml;
  }

  return std::unexpected(BindingError::kInvalidArg);
}

/// @brief Parses a SPARQL result format name.
/// @param format `"json"`, `"xml"`, or `"html"`
/// @return Format, or BindingError::kInvalidArg
[[nodiscard]] constexpr auto ParseResultFormat(std::string_view format) noexcept
    -> BindingResult<sparql::ResultFormat> {
  if (format == "json") [[likely]] {
    return sparql::ResultFormat::kJson;
  }

  if (format == "xml") {
    return sparql::ResultFormat::kXml;
  }

  if (format == "html") {
    return sparql::ResultFormat::kHtml;
  }

  return std::unexpected(BindingError::kInvalidArg);
}

}  // namespace cccr::bindings::python
