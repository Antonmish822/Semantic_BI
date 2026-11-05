#pragma once

#include <cccr/context.hpp>
#include <cccr/rdf/term.hpp>

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <vector>

namespace cccr::detail {

struct GraphData;

}  // namespace cccr::detail

namespace cccr::rdf {

/// @brief In-memory RDF graph backed by a Redland memory model.
/// @details Copies share the same underlying model and triple cache
///          (`std::shared_ptr`). Create independent graphs with
///          @ref FromContext / default construction, then @ref Add.
class Graph {
public:
  /// @brief Constructs an empty graph using `std::pmr::get_default_resource()`.
  Graph() : Graph(FromContext(Context{})) {}
  Graph(const Graph&) = default;
  Graph(Graph&&) noexcept = default;
  ~Graph() = default;

  Graph& operator=(const Graph&) = default;
  Graph& operator=(Graph&&) noexcept = default;

  /// @brief Returns an empty graph that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty graph bound to @p ctx
  /// @warning Asserts when Redland world/model creation fails
  [[nodiscard]] static Graph FromContext(Context ctx);

  /// @brief Appends a triple to the model and the PMR cache.
  /// @param triple Triple to store
  void Add(const Triple& triple);

  /// @brief Appends a triple to the model and the PMR cache.
  /// @param triple Triple to store
  void Add(Triple&& triple);

  /// @brief Appends all triples from @p other into this graph.
  /// @param other Source graph (must not alias this graph's shared data)
  void MergeFrom(const Graph& other);

  /// @brief Returns whether the graph has no triples.
  /// @return `true` if empty
  [[nodiscard]] bool Empty() const noexcept;

  /// @brief Returns the number of triples.
  /// @return Triple count
  [[nodiscard]] size_t Size() const noexcept;

  /// @brief Returns all triples (PMR cache, kept in sync with the model).
  /// @return Const reference to the triple vector
  [[nodiscard]] auto Triples() const noexcept
      -> const std::pmr::vector<Triple>&;

  /// @brief Returns the memory resource used by the triple cache.
  /// @return PMR resource pointer
  [[nodiscard]] auto Allocator() const noexcept -> std::pmr::memory_resource*;

  /// @brief Returns the shared Redland-backed payload (library-internal).
  /// @return Shared graph data
  [[nodiscard]] auto Data() noexcept -> std::shared_ptr<detail::GraphData>& {
    return data_;
  }

  /// @brief Returns the shared Redland-backed payload (library-internal).
  /// @return Shared graph data
  [[nodiscard]] auto Data() const noexcept
      -> const std::shared_ptr<detail::GraphData>& {
    return data_;
  }

private:
  explicit Graph(std::shared_ptr<detail::GraphData> data) noexcept
      : data_(std::move(data)) {}

  std::shared_ptr<detail::GraphData> data_;
};

}  // namespace cccr::rdf
