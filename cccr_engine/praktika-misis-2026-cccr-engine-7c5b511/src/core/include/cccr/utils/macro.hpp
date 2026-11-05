#pragma once

/// @brief Returns a bitmask with bit @p x set (0-based).
/// @param x Bit index
#define CCCR_BIT(x) (1 << (x))

/// @brief Stringifies @p x without expanding macros in @p x.
/// @param x Token to stringify
#define CCCR_STRINGIFY_IMPL(x) #x

/// @brief Stringifies the expansion of @p x.
/// @param x Macro or token to stringify
#define CCCR_STRINGIFY(x) CCCR_STRINGIFY_IMPL(x)

/// @brief Concatenates tokens @p a and @p b without expanding them first.
/// @param a Left token
/// @param b Right token
#define CCCR_CONCAT_IMPL(a, b) a##b

/// @brief Concatenates the expansions of @p a and @p b.
/// @param a Left macro or token
/// @param b Right macro or token
#define CCCR_CONCAT(a, b) CCCR_CONCAT_IMPL(a, b)

/// @brief Generates a unique identifier with @p prefix and the current line.
/// @param prefix Identifier prefix
#define CCCR_ANONYMOUS_VAR(prefix) CCCR_CONCAT(prefix, __LINE__)
