#pragma once

namespace cccr::input {

/// @brief Opaque handle to a node in an input document (non-owning).
/// @details Lifetime is owned by the @ref Source that produced it.
class NodeRef {
public:
  /// @brief Constructs an empty node reference.
  constexpr NodeRef() noexcept = default;

  /// @brief Constructs a reference to an existing backend node handle.
  /// @param handle Opaque backend pointer
  explicit constexpr NodeRef(void* handle) noexcept : handle_(handle) {}

  /// @brief Returns whether this reference is empty.
  /// @return `true` if no node is held
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return handle_ == nullptr;
  }

  /// @brief Returns the opaque backend handle.
  /// @return Pointer passed to format backends
  [[nodiscard]] constexpr void* Handle() const noexcept { return handle_; }

private:
  void* handle_ = nullptr;
};

}  // namespace cccr::input
