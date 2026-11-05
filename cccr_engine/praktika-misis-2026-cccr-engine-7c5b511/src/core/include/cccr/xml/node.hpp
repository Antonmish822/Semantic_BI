#pragma once

#include <cccr/input/node.hpp>

#include <utility>

namespace cccr::xml {

/// @brief Opaque handle to a Saxon XDM node (alias of @ref input::NodeRef).
using NodeRef = input::NodeRef;

class Processor;

/// @brief Owns a parsed XML document root node.
class Document {
public:
  /// @brief Constructs an empty document.
  Document() = default;

  /// @brief Moves ownership of the root node from @p other.
  /// @param other Document to move from
  Document(Document&& other) noexcept
      : root_(std::exchange(other.root_, nullptr)) {}
  Document(const Document&) = delete;

  /// @brief Destroys the owned Saxon document node.
  ~Document();

  Document& operator=(const Document&) = delete;

  /// @brief Move-assigns ownership of the root node from @p other.
  /// @param other Document to move from
  /// @return Reference to this document
  Document& operator=(Document&&) noexcept;

  /// @brief Returns whether the document is empty.
  /// @return `true` if no root node is owned
  [[nodiscard]] bool Empty() const noexcept { return root_ == nullptr; }

  /// @brief Returns a non-owning reference to the document root.
  /// @return Root node reference
  [[nodiscard]] NodeRef Root() const noexcept { return NodeRef{root_}; }

private:
  friend class Processor;
  explicit Document(void* root) noexcept : root_(root) {}

  void* root_ = nullptr;
};

}  // namespace cccr::xml
