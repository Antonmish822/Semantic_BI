#pragma once

#include <cccr/bindings/common/error.hpp>
#include <cccr/context.hpp>

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>

namespace cccr::bindings::python {

/// @brief Shared arena + core Context kept alive by all Python handles.
struct ContextState {
  static constexpr size_t kDefaultPoolSize = 1024 * 1024;

  /// @brief Constructs a state with an arena of @p arena_bytes (0 → default).
  /// @param arena_bytes Initial monotonic arena size in bytes
  explicit ContextState(size_t arena_bytes = 0)
      : arena(arena_bytes == 0 ? kDefaultPoolSize : arena_bytes),
        core{.allocator = &arena} {}

  void SetError(BindingError error) {
    last_error.assign(BindingErrorToString(error));
  }

  void SetError(std::string_view message) { last_error.assign(message); }

  void ClearError() { last_error.clear(); }

  std::pmr::monotonic_buffer_resource arena;
  cccr::Context core;
  std::string last_error;
};

using ContextStatePtr = std::shared_ptr<ContextState>;

}  // namespace cccr::bindings::python
