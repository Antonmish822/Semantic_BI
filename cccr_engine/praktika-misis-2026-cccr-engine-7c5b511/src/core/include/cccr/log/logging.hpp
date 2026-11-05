#pragma once

#include <cccr/context.hpp>
#include <cccr/log/level.hpp>

#include <cstdio>
#include <format>
#include <iterator>
#include <memory_resource>
#include <string_view>
#include <utility>

namespace cccr::log {

namespace detail {

/// @brief Writes `@p message` to stderr with a level prefix.
/// @param level Log severity
/// @param ctx Unused for the string_view path; kept for API symmetry with
///            formatted logging
/// @param message Message body
inline void Write(Level level, Context /*ctx*/,
                  std::string_view message) noexcept {
  const auto prefix = LevelToString(level);
  std::fprintf(stderr, "[%.*s] %.*s\n", static_cast<int>(prefix.size()),
               prefix.data(), static_cast<int>(message.size()), message.data());
}

/// @brief Formats @p fmt into a PMR string via @p ctx and forwards to @ref
/// Write.
/// @tparam Args Format argument types
/// @param level Log severity
/// @param ctx Allocator source for the temporary message buffer
/// @param fmt Format string
/// @param args Format arguments
template <typename... Args>
inline void WriteFormatted(Level level, Context ctx,
                           std::format_string<Args...> fmt,
                           Args&&... args) noexcept {
  try {
    std::pmr::string message{ctx.GetAllocator()};
    std::format_to(std::back_inserter(message), fmt,
                   std::forward<Args>(args)...);
    Write(level, ctx, message);
  } catch (...) {
    Write(level, ctx, "Failed to format log message");
  }
}

}  // namespace detail

#if !defined(NDEBUG)

/// @brief Logs a debug message using @p ctx.
/// @param ctx Runtime context
/// @param message The message to log
inline void Debug(Context ctx, std::string_view message) noexcept {
  detail::Write(Level::kDebug, ctx, message);
}

/// @brief Logs a debug message with a default-constructed @ref Context.
/// @param message The message to log
inline void Debug(std::string_view message) noexcept {
  Debug(Context{}, message);
}

/// @brief Logs a formatted debug message using @p ctx's allocator.
/// @tparam Args Types of the format arguments
/// @param ctx Runtime context (PMR allocator for the message buffer)
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Debug(Context ctx, std::format_string<Args...> fmt,
                  Args&&... args) noexcept {
  detail::WriteFormatted(Level::kDebug, ctx, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a formatted debug message with a default-constructed @ref
/// Context.
/// @tparam Args Types of the format arguments
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Debug(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Debug(Context{}, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a trace message using @p ctx.
/// @param ctx Runtime context
/// @param message The message to log
inline void Trace(Context ctx, std::string_view message) noexcept {
  detail::Write(Level::kTrace, ctx, message);
}

/// @brief Logs a trace message with a default-constructed @ref Context.
/// @param message The message to log
inline void Trace(std::string_view message) noexcept {
  Trace(Context{}, message);
}

/// @brief Logs a formatted trace message using @p ctx's allocator.
/// @tparam Args Types of the format arguments
/// @param ctx Runtime context (PMR allocator for the message buffer)
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Trace(Context ctx, std::format_string<Args...> fmt,
                  Args&&... args) noexcept {
  detail::WriteFormatted(Level::kTrace, ctx, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a formatted trace message with a default-constructed @ref
/// Context.
/// @tparam Args Types of the format arguments
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Trace(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Trace(Context{}, fmt, std::forward<Args>(args)...);
}

#else

inline void Debug(Context /*ctx*/, std::string_view /*message*/) noexcept {}

inline void Debug(std::string_view message) noexcept {
  Debug(Context{}, message);
}

template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Debug(Context /*ctx*/, std::format_string<Args...> /*fmt*/,
                  Args&&... /*args*/) noexcept {}

template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Debug(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Debug(Context{}, fmt, std::forward<Args>(args)...);
}

inline void Trace(Context /*ctx*/, std::string_view /*message*/) noexcept {}

inline void Trace(std::string_view message) noexcept {
  Trace(Context{}, message);
}

template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Trace(Context /*ctx*/, std::format_string<Args...> /*fmt*/,
                  Args&&... /*args*/) noexcept {}

template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Trace(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Trace(Context{}, fmt, std::forward<Args>(args)...);
}

#endif

/// @brief Logs an info message using @p ctx.
/// @param ctx Runtime context
/// @param message The message to log
inline void Info(Context ctx, std::string_view message) noexcept {
  detail::Write(Level::kInfo, ctx, message);
}

/// @brief Logs an info message with a default-constructed @ref Context.
/// @param message The message to log
inline void Info(std::string_view message) noexcept {
  Info(Context{}, message);
}

/// @brief Logs a formatted info message using @p ctx's allocator.
/// @tparam Args Types of the format arguments
/// @param ctx Runtime context (PMR allocator for the message buffer)
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Info(Context ctx, std::format_string<Args...> fmt,
                 Args&&... args) noexcept {
  detail::WriteFormatted(Level::kInfo, ctx, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a formatted info message with a default-constructed @ref
/// Context.
/// @tparam Args Types of the format arguments
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Info(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Info(Context{}, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a warning message using @p ctx.
/// @param ctx Runtime context
/// @param message The message to log
inline void Warn(Context ctx, std::string_view message) noexcept {
  detail::Write(Level::kWarn, ctx, message);
}

/// @brief Logs a warning message with a default-constructed @ref Context.
/// @param message The message to log
inline void Warn(std::string_view message) noexcept {
  Warn(Context{}, message);
}

/// @brief Logs a formatted warning message using @p ctx's allocator.
/// @tparam Args Types of the format arguments
/// @param ctx Runtime context (PMR allocator for the message buffer)
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Warn(Context ctx, std::format_string<Args...> fmt,
                 Args&&... args) noexcept {
  detail::WriteFormatted(Level::kWarn, ctx, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a formatted warning message with a default-constructed @ref
/// Context.
/// @tparam Args Types of the format arguments
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Warn(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Warn(Context{}, fmt, std::forward<Args>(args)...);
}

/// @brief Logs an error message using @p ctx.
/// @param ctx Runtime context
/// @param message The message to log
inline void Error(Context ctx, std::string_view message) noexcept {
  detail::Write(Level::kError, ctx, message);
}

/// @brief Logs an error message with a default-constructed @ref Context.
/// @param message The message to log
inline void Error(std::string_view message) noexcept {
  Error(Context{}, message);
}

/// @brief Logs a formatted error message using @p ctx's allocator.
/// @tparam Args Types of the format arguments
/// @param ctx Runtime context (PMR allocator for the message buffer)
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Error(Context ctx, std::format_string<Args...> fmt,
                  Args&&... args) noexcept {
  detail::WriteFormatted(Level::kError, ctx, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a formatted error message with a default-constructed @ref
/// Context.
/// @tparam Args Types of the format arguments
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Error(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Error(Context{}, fmt, std::forward<Args>(args)...);
}

/// @brief Logs a critical message using @p ctx.
/// @param ctx Runtime context
/// @param message The message to log
inline void Critical(Context ctx, std::string_view message) noexcept {
  detail::Write(Level::kCritical, ctx, message);
}

/// @brief Logs a critical message with a default-constructed @ref Context.
/// @param message The message to log
inline void Critical(std::string_view message) noexcept {
  Critical(Context{}, message);
}

/// @brief Logs a formatted critical message using @p ctx's allocator.
/// @tparam Args Types of the format arguments
/// @param ctx Runtime context (PMR allocator for the message buffer)
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Critical(Context ctx, std::format_string<Args...> fmt,
                     Args&&... args) noexcept {
  detail::WriteFormatted(Level::kCritical, ctx, fmt,
                         std::forward<Args>(args)...);
}

/// @brief Logs a formatted critical message with a default-constructed
///        @ref Context.
/// @tparam Args Types of the format arguments
/// @param fmt Format string
/// @param args Arguments for the format string
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Critical(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Critical(Context{}, fmt, std::forward<Args>(args)...);
}

}  // namespace cccr::log
