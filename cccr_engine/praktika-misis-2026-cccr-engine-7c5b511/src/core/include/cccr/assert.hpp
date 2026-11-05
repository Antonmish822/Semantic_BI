#pragma once

#include <cccr/compiler.hpp>
#include <cccr/platform.hpp>
#include <cccr/utils/macro.hpp>

#include <format>
#include <source_location>
#include <string>
#include <string_view>

namespace cccr {

/// @brief Function signature for custom assertion handlers.
/// @param condition The failed condition as a string
/// @param loc Source location of the assertion
/// @param message Additional message (may be empty)
using AssertionHandler = void (*)(std::string_view condition,
                                  const std::source_location& loc,
                                  std::string_view message) noexcept;

/// @brief Default assertion handler (`nullptr` means use built-in default
/// behavior).
inline constexpr AssertionHandler kDefaultAssertionHandler = nullptr;

namespace details {

#ifdef CCCR_ENABLE_ASSERTS
inline constexpr bool kEnableAssert = true;
#else
inline constexpr bool kEnableAssert = false;
#endif

/// @brief Storage for the custom assertion handler.
/// @details When set, this handler is called instead of the default behavior.
/// The handler can be set by calling `SetAssertionHandler()`.
inline AssertionHandler g_custom_assertion_handler = nullptr;

/// @brief Default assertion handler that prints using spdlog.
/// @param condition The failed condition as a string
/// @param loc Source location of the assertion
/// @param message Additional message
void DefaultAssertionHandler(std::string_view condition,
                             const std::source_location& loc,
                             std::string_view message) noexcept;

/// @brief Unified assertion handling function.
/// @details Priority order:
///   1. Custom handler (if set by user via SetAssertionHandler)
///   2. Default handler
///
/// @param condition The failed condition as a string
/// @param loc Source location of the assertion
/// @param message Additional message
inline void HandleAssertion(std::string_view condition,
                            const std::source_location& loc,
                            std::string_view message) noexcept {
  // Priority 1: Custom user handler
  if (g_custom_assertion_handler != nullptr) {
    g_custom_assertion_handler(condition, loc, message);
    return;
  }

  // Priority 2: Default handler
  DefaultAssertionHandler(condition, loc, message);
}

}  // namespace details

/// @brief Sets a custom assertion handler.
/// @details When set, this handler is called for all assertion failures instead
/// of the default behavior. Set to `nullptr` to restore default behavior.
/// @param handler The custom handler function, or `nullptr` to use default
///
/// @code
/// // Set custom handler
/// cccr::SetAssertionHandler(
///     [](std::string_view condition, const std::source_location& loc,
///        std::string_view message) noexcept {
///   // Your custom logging here
///   std::fprintf(stderr, "Assert: %.*s | %.*s [%s:%u]\n",
///                static_cast<int>(condition.size()), condition.data(),
///                static_cast<int>(message.size()), message.data(),
///                loc.file_name(), loc.line());
/// });
///
/// // Reset to default behavior
/// cccr::SetAssertionHandler(cccr::kDefaultAssertionHandler);
/// @endcode
inline void SetAssertionHandler(AssertionHandler handler) noexcept {
  details::g_custom_assertion_handler = handler;
}

/// @brief Gets the current custom assertion handler.
/// @return The current custom handler, or `nullptr` if using default behavior
[[nodiscard]] inline AssertionHandler GetAssertionHandler() noexcept {
  return details::g_custom_assertion_handler;
}

/// @brief Formats a complete assertion failure message with source location and
/// stack trace.
/// @param condition The failed condition as a string
/// @param loc Source location of the assertion
/// @param message Additional message (may be empty)
/// @return Formatted assertion message
[[nodiscard]] std::string FormatAssertionMessage(
    std::string_view condition, const std::source_location& loc,
    std::string_view message);

}  // namespace cccr

// NOLINTBEGIN(cppcoreguidelines-avoid-do-while)
// NOLINTBEGIN(cppcoreguidelines-macro-usage)

/// @brief Assertion macro that aborts execution in debug builds.
/// @details Does nothing in release builds.
/// Uses the configured assertion handler (custom -> log plugin -> default).
/// Supports format strings and arguments.
/// @param condition The condition to check
/// @param ... Optional message (can be format string with arguments)
/// @hideinitializer
#ifdef CCCR_ENABLE_ASSERTS
#define CCCR_ASSERT(condition, ...)                                  \
  do {                                                               \
    if constexpr (::cccr::details::kEnableAssert) {                  \
      if (CCCR_EXPECT_FALSE(!(condition))) [[unlikely]] {            \
        if constexpr (sizeof(#__VA_ARGS__) > 1) {                    \
          try {                                                      \
            const std::string msg = std::format("" __VA_ARGS__);     \
            ::cccr::details::HandleAssertion(                        \
                #condition, ::std::source_location::current(), msg); \
          } catch (...) {                                            \
            ::cccr::details::HandleAssertion(                        \
                #condition, ::std::source_location::current(),       \
                "Formatting error in assertion");                    \
          }                                                          \
        } else {                                                     \
          ::cccr::details::HandleAssertion(                          \
              #condition, ::std::source_location::current(), "");    \
        }                                                            \
        CCCR_DEBUG_BREAK();                                          \
      }                                                              \
    }                                                                \
  } while (false)
#else
#define CCCR_ASSERT(condition, ...) \
  [[maybe_unused]] static constexpr auto CCCR_ANONYMOUS_VAR(unused_assert) = 0
#endif

/// @brief Invariant check that asserts in debug builds and logs error in
/// release.
/// @details Provides runtime safety checks that are enforced even in release
/// builds. In debug builds, triggers assertion. In release builds, logs error
/// and continues.
/// @param condition The condition to check
/// @param ... Optional message (can be format string with arguments)
/// @hideinitializer
#ifdef CCCR_ENABLE_ASSERTS
#define CCCR_INVARIANT(condition, ...)                                        \
  do {                                                                        \
    if (CCCR_EXPECT_FALSE(!(condition))) [[unlikely]] {                       \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                               \
        try {                                                                 \
          const std::string msg = std::format("" __VA_ARGS__);                \
          ::cccr::details::HandleAssertion(                                   \
              #condition, ::std::source_location::current(), msg);            \
        } catch (...) {                                                       \
          ::cccr::details::HandleAssertion(#condition,                        \
                                           ::std::source_location::current(), \
                                           "Formatting error in invariant");  \
        }                                                                     \
      } else {                                                                \
        ::cccr::details::HandleAssertion(                                     \
            #condition, ::std::source_location::current(), "");               \
      }                                                                       \
      CCCR_DEBUG_BREAK();                                                     \
    }                                                                         \
  } while (false)
#else
#define CCCR_INVARIANT(condition, ...)                             \
  do {                                                             \
    if (CCCR_EXPECT_FALSE(!(condition))) [[unlikely]] {            \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                    \
        try {                                                      \
          const std::string msg = std::format("" __VA_ARGS__);     \
          ::cccr::details::HandleAssertion(                        \
              #condition, ::std::source_location::current(), msg); \
        } catch (...) {                                            \
          ::cccr::details::HandleAssertion(                        \
              #condition, ::std::source_location::current(), "");  \
        }                                                          \
      } else {                                                     \
        ::cccr::details::HandleAssertion(                          \
            #condition, ::std::source_location::current(), "");    \
      }                                                            \
    }                                                              \
  } while (false)
#endif

/// @brief Verify macro that always checks the condition.
/// @details Similar to assert but runs in both debug and release builds.
/// Useful for validating external input or critical invariants.
/// @param condition The condition to check
/// @param ... Optional message (can be format string with arguments)
/// @hideinitializer
#define CCCR_VERIFY(condition, ...)                                           \
  do {                                                                        \
    if (CCCR_EXPECT_FALSE(!(condition))) [[unlikely]] {                       \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                               \
        try {                                                                 \
          const std::string msg = std::format("" __VA_ARGS__);                \
          ::cccr::details::HandleAssertion(                                   \
              #condition, ::std::source_location::current(), msg);            \
        } catch (...) {                                                       \
          ::cccr::details::HandleAssertion(#condition,                        \
                                           ::std::source_location::current(), \
                                           "Formatting error in verify");     \
        }                                                                     \
      } else {                                                                \
        ::cccr::details::HandleAssertion(                                     \
            #condition, ::std::source_location::current(), "");               \
      }                                                                       \
      CCCR_DEBUG_BREAK();                                                     \
    }                                                                         \
  } while (false)

// NOLINTEND(cppcoreguidelines-macro-usage)
// NOLINTEND(cppcoreguidelines-avoid-do-while)
