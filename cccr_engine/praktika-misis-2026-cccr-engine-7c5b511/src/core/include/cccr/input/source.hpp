#pragma once

#include <cccr/input/error.hpp>
#include <cccr/input/node.hpp>
#include <cccr/input/query.hpp>

#include <string>
#include <vector>

namespace cccr::input {

/// @brief Format-agnostic input document interface.
/// @details XML/HTML/CSV backends implement this so @ref cccr::engine and
///          @ref cccr::eval stay free of format-specific types. @ref Query
///          strings are interpreted by the backend (XQuery for XML, etc.).
class Source {
public:
  /// @brief Constructs an empty source base.
  Source() = default;
  Source(const Source&) = delete;
  Source(Source&&) = delete;

  /// @brief Destroys the source.
  virtual ~Source() = default;

  Source& operator=(const Source&) = delete;
  Source& operator=(Source&&) = delete;

  /// @brief Evaluates @p query to a string against @p context.
  /// @param context Context node
  /// @param query Backend query expression
  /// @return String value or InputError
  [[nodiscard]] virtual auto EvalString(NodeRef context, Query query)
      -> InputResult<std::pmr::string> = 0;

  /// @brief Evaluates @p query to a node sequence against @p context.
  /// @param context Context node
  /// @param query Backend query expression
  /// @return Nodes (valid for this Source's lifetime) or InputError
  [[nodiscard]] virtual auto EvalNodes(NodeRef context, Query query)
      -> InputResult<std::pmr::vector<NodeRef>> = 0;

  /// @brief Returns the document root (or equivalent top-level node).
  /// @return Root node reference
  [[nodiscard]] virtual NodeRef Root() const noexcept = 0;
};

}  // namespace cccr::input
