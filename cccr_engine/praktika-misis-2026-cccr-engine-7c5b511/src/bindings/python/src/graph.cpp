#include "graph.hpp"

#include "util.hpp"

#include <cccr/bindings/common/error.hpp>
#include <cccr/bindings/common/pipeline.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace cccr::bindings::python {

auto Graph::ToRdfXml() const -> std::optional<std::string> {
  auto serialized = SerializeRdfXml(graph_);
  if (!serialized) [[unlikely]] {
    state_->SetError(serialized.error());
    return std::nullopt;
  }
  state_->ClearError();
  return std::move(*serialized);
}

auto Graph::Query(std::string_view sparql, std::string_view result_format) const
    -> std::optional<std::string> {
  const auto format = ParseResultFormat(result_format);
  if (!format) [[unlikely]] {
    state_->SetError(format.error());
    return std::nullopt;
  }

  auto result = QueryGraph(state_->core, graph_, sparql, *format);
  if (!result) [[unlikely]] {
    state_->SetError(result.error());
    return std::nullopt;
  }

  state_->ClearError();
  return std::move(*result);
}

bool Graph::Merge(const Graph& other) {
  if (state_ != other.state_) [[unlikely]] {
    state_->SetError(BindingError::kInvalidArg);
    return false;
  }

  auto merged = MergeGraph(graph_, other.graph_);
  if (!merged) [[unlikely]] {
    state_->SetError(merged.error());
    return false;
  }

  state_->ClearError();
  return true;
}

}  // namespace cccr::bindings::python
