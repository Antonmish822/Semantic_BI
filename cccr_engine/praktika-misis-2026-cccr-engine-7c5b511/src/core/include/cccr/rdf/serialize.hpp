#pragma once

#include <cccr/rdf/graph.hpp>

#include <memory_resource>

namespace cccr::rdf {

/// @brief Serializes @p graph to RDF/XML via Redland/raptor.
/// @details Allocates the result using @p graph's allocator.
/// @param graph Graph to serialize
/// @return RDF/XML string, or empty on failure
[[nodiscard]] std::pmr::string ToRdfXml(const Graph& graph);

}  // namespace cccr::rdf
