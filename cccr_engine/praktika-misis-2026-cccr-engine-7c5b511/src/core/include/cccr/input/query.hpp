#pragma once

#include <string_view>

namespace cccr::input {

/// @brief Backend query expression (XQuery, path, etc.).
/// @details Interpretation is owned by the @ref Source implementation. Prefer
///          @ref From so arbitrary strings are not treated as queries by
///          accident.
class Query {
public:
  /// @brief Returns a query wrapping @p text.
  /// @param text Query expression text (not owned)
  /// @return Query view
  [[nodiscard]] static constexpr Query From(std::string_view text) noexcept {
    return Query{text};
  }

  /// @brief Returns whether the query text is empty.
  /// @return `true` if empty
  [[nodiscard]] constexpr bool Empty() const noexcept { return text_.empty(); }

  /// @brief Returns the query expression text.
  /// @return Non-owning view of the expression
  [[nodiscard]] constexpr std::string_view Text() const noexcept {
    return text_;
  }

private:
  explicit constexpr Query(std::string_view text) noexcept : text_(text) {}

  std::string_view text_;
};

}  // namespace cccr::input
