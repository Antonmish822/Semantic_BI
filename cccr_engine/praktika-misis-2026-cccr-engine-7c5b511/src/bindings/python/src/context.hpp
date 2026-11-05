#pragma once

#include "context_state.hpp"
#include "graph.hpp"
#include "schema.hpp"
#include "source.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace cccr::bindings::python {

class Context {
public:
  /// @brief Creates a context with an arena of @p arena_size bytes (0 →
  /// default).
  /// @param arena_size Initial monotonic arena size in bytes
  explicit Context(size_t arena_size = 0)
      : state_(std::make_shared<ContextState>(arena_size)) {}

  /// @brief Loads a CCCR schema from JSON.
  /// @return Schema, or nullopt (see @ref LastError)
  [[nodiscard]] auto LoadSchema(std::string_view json,
                                std::optional<std::string_view> include_base =
                                    std::nullopt) -> std::optional<Schema>;

  /// @brief Loads an XML/HTML document.
  /// @return Source, or nullopt (see @ref LastError)
  [[nodiscard]] auto LoadXml(std::string_view xml) -> std::optional<Source>;

  /// @brief Loads a CSV document.
  /// @return Source, or nullopt (see @ref LastError)
  [[nodiscard]] auto LoadCsv(std::string_view csv) -> std::optional<Source>;

  /// @brief Loads an RDF graph from text.
  /// @return Graph, or nullopt (see @ref LastError)
  [[nodiscard]] auto LoadGraph(std::string_view text,
                               std::string_view format = "ntriples")
      -> std::optional<Graph>;

  /// @brief Runs @p schema over @p source.
  /// @return Output graph, or nullopt (see @ref LastError)
  [[nodiscard]] auto Run(const Schema& schema, Source& source,
                         const Graph* knowledge = nullptr)
      -> std::optional<Graph>;

  /// @brief Runs @p schema over each source and merges outputs.
  /// @return Merged output graph, or nullopt (see @ref LastError)
  [[nodiscard]] auto RunMany(const Schema& schema,
                             std::span<std::reference_wrapper<Source>> sources,
                             const Graph* knowledge = nullptr)
      -> std::optional<Graph>;

  [[nodiscard]] const ContextStatePtr& State() const noexcept { return state_; }

  [[nodiscard]] std::string_view LastError() const noexcept {
    return state_->last_error;
  }

  void ClearError() noexcept { state_->ClearError(); }

private:
  ContextStatePtr state_;
};

}  // namespace cccr::bindings::python
