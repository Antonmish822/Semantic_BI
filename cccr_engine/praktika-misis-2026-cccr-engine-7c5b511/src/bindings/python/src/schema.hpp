#pragma once

#include "context_state.hpp"

#include <cccr/ast/types.hpp>

#include <filesystem>
#include <utility>

namespace cccr::bindings::python {

class Schema {
public:
  Schema(ContextStatePtr state, ast::Schema schema,
         std::filesystem::path include_base);

  [[nodiscard]] const ast::Schema& Get() const noexcept { return schema_; }

  [[nodiscard]] const std::filesystem::path& IncludeBase() const noexcept {
    return include_base_;
  }

  [[nodiscard]] const ContextStatePtr& State() const noexcept { return state_; }

private:
  ContextStatePtr state_;
  ast::Schema schema_;
  std::filesystem::path include_base_;
};

inline Schema::Schema(ContextStatePtr state, ast::Schema schema,
                      std::filesystem::path include_base)
    : state_(std::move(state)),
      schema_(std::move(schema)),
      include_base_(std::move(include_base)) {}

}  // namespace cccr::bindings::python
