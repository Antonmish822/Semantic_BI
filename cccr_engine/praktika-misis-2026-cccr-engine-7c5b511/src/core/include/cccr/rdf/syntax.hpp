#pragma once

#include <cstdint>
#include <string_view>

namespace cccr::rdf {

/// @brief Concrete RDF serialization syntaxes supported for graph load.
enum class RdfSyntax : uint8_t {
  /// @brief N-Triples (`ntriples` raptor parser).
  kNTriples,
  /// @brief Turtle (`turtle` raptor parser).
  kTurtle,
  /// @brief RDF/XML (`rdfxml` raptor parser).
  kRdfXml,
};

/// @brief Returns the Redland/raptor parser name for @p syntax.
/// @param syntax RDF syntax
/// @return Non-empty parser name
[[nodiscard]] constexpr std::string_view RdfSyntaxToRaptorName(
    RdfSyntax syntax) noexcept {
  switch (syntax) {
    using enum RdfSyntax;
    case kNTriples:
      return "ntriples";
    case kTurtle:
      return "turtle";
    case kRdfXml:
      return "rdfxml";
    default:
      return "ntriples";
  }
}

/// @brief Converts @p syntax to a short diagnostic label.
/// @param syntax RDF syntax
/// @return Human-readable name
[[nodiscard]] constexpr std::string_view RdfSyntaxToString(
    RdfSyntax syntax) noexcept {
  switch (syntax) {
    using enum RdfSyntax;
    case kNTriples:
      return "N-Triples";
    case kTurtle:
      return "Turtle";
    case kRdfXml:
      return "RDF/XML";
    default:
      return "Unknown RDF syntax";
  }
}

}  // namespace cccr::rdf
