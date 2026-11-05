#include <pch.hpp>

#include <cccr/rdf/serialize.hpp>

#include <cccr/rdf/graph.hpp>

#include <detail/redland_store.hpp>

#include <memory_resource>
#include <utility>

namespace cccr::rdf {

std::pmr::string ToRdfXml(const Graph& graph) {
  auto* const mr = graph.Allocator();
  auto xml = graph.Data()->store.SerializeRdfXml(mr);
  if (!xml) [[unlikely]] {
    return std::pmr::string{mr};
  }
  return std::move(*xml);
}

}  // namespace cccr::rdf
