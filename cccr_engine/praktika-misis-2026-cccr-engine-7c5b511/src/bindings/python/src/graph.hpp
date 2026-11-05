#pragma once

#include "context_state.hpp"

#include <cccr/rdf/graph.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace cccr::bindings::python {

class Graph {
public:
  Graph(ContextStatePtr state, rdf::Graph graph)
      : state_(std::move(state)), graph_(std::move(graph)) {}

  [[nodiscard]] rdf::Graph& Get() noexcept { return graph_; }

  [[nodiscard]] const rdf::Graph& Get() const noexcept { return graph_; }

  [[nodiscard]] const ContextStatePtr& State() const noexcept { return state_; }

  /// @brief Serializes to RDF/XML.
  /// @return RDF/XML text, or nullopt (see Context.last_error)
  [[nodiscard]] auto ToRdfXml() const -> std::optional<std::string>;

  /// @brief Runs a SPARQL SELECT over this graph.
  /// @param sparql Query text
  /// @param result_format `"json"`, `"xml"`, or `"html"`
  /// @return Formatted results, or nullopt (see Context.last_error)
  [[nodiscard]] auto Query(std::string_view sparql,
                           std::string_view result_format = "json") const
      -> std::optional<std::string>;

  /// @brief Appends all triples from @p other into this graph.
  /// @param other Source graph (same Context; must not alias this graph)
  /// @return `true` on success; on failure see Context.last_error
  [[nodiscard]] bool Merge(const Graph& other);

private:
  ContextStatePtr state_;
  rdf::Graph graph_;
};

}  // namespace cccr::bindings::python
