#pragma once

#include <cccr/context.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory_resource>
#include <string>
#include <string_view>

namespace cccr::utils {

/// @brief Errors from filesystem helpers.
enum class FileError : uint8_t {
  /// @brief Path could not be opened for reading or writing.
  kCouldNotOpen,
  /// @brief Opened file could not be read completely.
  kReadError,
  /// @brief Opened file could not be written completely.
  kWriteError,
};

/// @brief Result alias for file operations.
/// @tparam T Success value type
template <typename T>
using FileResult = std::expected<T, FileError>;

/// @brief Converts @p error to a human-readable string.
/// @param error The error code
/// @return Human-readable message
[[nodiscard]] constexpr std::string_view FileErrorToString(
    FileError error) noexcept {
  switch (error) {
    using enum FileError;
    case kCouldNotOpen:
      return "Could not open file";
    case kReadError:
      return "Could not read file";
    case kWriteError:
      return "Could not write file";
    default:
      return "Unknown file error";
  }
}

/// @brief Reads the entire contents of a file into a string.
/// @param filepath The path to the file
/// @param ctx Shared allocator context
/// @return An expected containing the file contents or a FileError
[[nodiscard]] inline auto ReadFileToString(std::string_view filepath,
                                           Context ctx = {})
    -> FileResult<std::pmr::string> {
  if (filepath.empty()) [[unlikely]] {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  std::ifstream in(filepath.data(), std::ios::in | std::ios::binary);
  if (!in) [[unlikely]] {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  auto* const mr = ctx.GetAllocator();
  std::pmr::string result{mr};
  in.seekg(0, std::ios::end);
  const auto pos = in.tellg();
  if (pos == std::ifstream::pos_type(-1)) [[unlikely]] {
    return std::unexpected(FileError::kReadError);
  }

  const auto size = static_cast<size_t>(pos);
  result.resize(size);
  in.seekg(0, std::ios::beg);
  in.read(result.data(), static_cast<std::streamsize>(result.size()));
  if (!in) [[unlikely]] {
    return std::unexpected(FileError::kReadError);
  }
  in.close();

  return result;
}

/// @brief Reads the entire contents of a file into a string.
/// @param filepath The path to the file
/// @param ctx Shared allocator context
/// @return An expected containing the file contents or a FileError
[[nodiscard]] inline auto ReadFileToString(
    const std::filesystem::path& filepath, Context ctx = {})
    -> FileResult<std::pmr::string> {
  if (filepath.empty()) [[unlikely]] {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  std::ifstream in(filepath, std::ios::in | std::ios::binary);
  if (!in) [[unlikely]] {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  auto* const mr = ctx.GetAllocator();
  std::pmr::string result{mr};
  in.seekg(0, std::ios::end);
  const auto pos = in.tellg();
  if (pos == std::ifstream::pos_type(-1)) [[unlikely]] {
    return std::unexpected(FileError::kReadError);
  }

  const auto size = static_cast<size_t>(pos);
  result.resize(size);
  in.seekg(0, std::ios::beg);
  in.read(result.data(), static_cast<std::streamsize>(result.size()));
  if (!in) [[unlikely]] {
    return std::unexpected(FileError::kReadError);
  }
  in.close();

  return result;
}

/// @brief Writes @p contents to @p filepath, replacing any existing file.
/// @param filepath Destination path
/// @param contents Bytes to write
/// @return Empty success or FileError
[[nodiscard]] inline auto WriteStringToFile(std::string_view filepath,
                                            std::string_view contents)
    -> FileResult<void> {
  if (filepath.empty()) [[unlikely]] {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  std::ofstream out(filepath.data(),
                    std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out) [[unlikely]] {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
  if (!out) [[unlikely]] {
    return std::unexpected(FileError::kWriteError);
  }

  out.close();
  if (!out) [[unlikely]] {
    return std::unexpected(FileError::kWriteError);
  }
  return {};
}

/// @brief Writes @p contents to @p filepath, replacing any existing file.
/// @param filepath Destination path
/// @param contents Bytes to write
/// @return Empty success or FileError
[[nodiscard]] inline auto WriteStringToFile(
    const std::filesystem::path& filepath, std::string_view contents)
    -> FileResult<void> {
  if (filepath.empty()) [[unlikely]] {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  std::ofstream out(filepath,
                    std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out) [[unlikely]] {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
  if (!out) [[unlikely]] {
    return std::unexpected(FileError::kWriteError);
  }

  out.close();
  if (!out) [[unlikely]] {
    return std::unexpected(FileError::kWriteError);
  }
  return {};
}

/// @brief Extracts the file name from a given path.
/// @param path The full file path
/// @return The file name
[[nodiscard]] constexpr std::string_view GetFileName(std::string_view path) {
  const size_t last_slash = path.find_last_of("/\\");
  return (last_slash != std::string_view::npos) ? path.substr(last_slash + 1)
                                                : path;
}

/// @brief Extracts the file extension from a given path.
/// @param path The full file path
/// @return The file extension, including the dot (e.g., ".txt"), or an empty
/// string if none exists
[[nodiscard]] constexpr std::string_view GetFileExtension(
    std::string_view path) {
  const size_t last_dot = path.find_last_of('.');
  return (last_dot != std::string_view::npos) ? path.substr(last_dot) : "";
}

}  // namespace cccr::utils
