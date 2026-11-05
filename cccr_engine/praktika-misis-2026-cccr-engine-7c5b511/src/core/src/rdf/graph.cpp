#include <pch.hpp>

#include <cccr/rdf/graph.hpp>

#include <cccr/assert.hpp>
#include <cccr/context.hpp>
#include <cccr/rdf/term.hpp>

#include <detail/redland_store.hpp>

#include <memory>
#include <memory_resource>
#include <utility>

namespace cccr::rdf {

Graph Graph::FromContext(Context ctx) {
  auto* const mr = ctx.GetAllocator();
  auto store = detail::RedlandStore::Create(mr);
  CCCR_ASSERT(store.has_value(), "Failed to create Redland memory store!");

  auto data = std::allocate_shared<detail::GraphData>(
      std::pmr::polymorphic_allocator<detail::GraphData>{mr}, std::move(*store),
      mr);
  return Graph{std::move(data)};
}

void Graph::Add(const Triple& triple) {
  CCCR_ASSERT(data_ != nullptr, "Graph has no backing store!");

  const bool ok = data_->store.Add(triple);
  CCCR_VERIFY(ok, "Failed to add triple to Redland model");
  if (!ok) [[unlikely]] {
    return;
  }
  data_->triples.push_back(triple);
}

void Graph::Add(Triple&& triple) {
  CCCR_ASSERT(data_ != nullptr, "Graph has no backing store!");

  const bool ok = data_->store.Add(triple);
  CCCR_VERIFY(ok, "Failed to add triple to Redland model");
  if (!ok) [[unlikely]] {
    return;
  }
  data_->triples.push_back(std::move(triple));
}

void Graph::MergeFrom(const Graph& other) {
  CCCR_ASSERT(data_ != nullptr, "Graph has no backing store!");
  CCCR_ASSERT(other.data_ != nullptr, "Other graph has no backing store!");
  CCCR_ASSERT(data_ != other.data_,
              "Cannot MergeFrom the same graph instance!");

  for (const auto& triple : other.Triples()) {
    Add(triple);
  }
}

bool Graph::Empty() const noexcept {
  CCCR_ASSERT(data_ != nullptr, "Graph has no backing store!");
  return data_->triples.empty();
}

size_t Graph::Size() const noexcept {
  CCCR_ASSERT(data_ != nullptr, "Graph has no backing store!");
  return data_->triples.size();
}

auto Graph::Triples() const noexcept -> const std::pmr::vector<Triple>& {
  CCCR_ASSERT(data_ != nullptr, "Graph has no backing store!");
  return data_->triples;
}

auto Graph::Allocator() const noexcept -> std::pmr::memory_resource* {
  CCCR_ASSERT(data_ != nullptr, "Graph has no backing store!");
  return data_->triples.get_allocator().resource();
}

}  // namespace cccr::rdf
