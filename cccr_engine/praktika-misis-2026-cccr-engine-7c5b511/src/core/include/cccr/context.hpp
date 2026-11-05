#pragma once

#include <memory_resource>

namespace cccr {

/// @brief Shared runtime context (allocator) for CCCR core APIs.
struct Context {
  /// @brief PMR resource for allocations (default:
  /// `std::pmr::get_default_resource()`).
  std::pmr::memory_resource* allocator = std::pmr::get_default_resource();

  /// @brief Returns a non-null allocator.
  /// @return @ref allocator, or the default resource when null
  [[nodiscard]] auto GetAllocator() const noexcept
      -> std::pmr::memory_resource* {
    return allocator != nullptr ? allocator : std::pmr::get_default_resource();
  }
};

}  // namespace cccr
