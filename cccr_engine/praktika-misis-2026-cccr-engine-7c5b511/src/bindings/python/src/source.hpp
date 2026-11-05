#pragma once

#include "context_state.hpp"

#include <cccr/input/source.hpp>

#include <memory>
#include <utility>

namespace cccr::bindings::python {

class Source {
public:
  Source(ContextStatePtr state, std::unique_ptr<input::Source> source)
      : state_(std::move(state)), source_(std::move(source)) {}

  [[nodiscard]] input::Source& Get() noexcept { return *source_; }

  [[nodiscard]] const ContextStatePtr& State() const noexcept { return state_; }

private:
  ContextStatePtr state_;
  std::unique_ptr<input::Source> source_;
};

}  // namespace cccr::bindings::python
