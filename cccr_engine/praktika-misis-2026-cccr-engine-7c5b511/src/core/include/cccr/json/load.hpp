#pragma once

#include <cccr/ast/types.hpp>
#include <cccr/context.hpp>
#include <cccr/json/error.hpp>

#include <filesystem>
#include <string_view>

namespace cccr::json {

/// @brief Optional parameters for @ref LoadSchema.
struct LoadSchemaParams {
  /// @brief Directory for resolving relative property `include`s.
  std::filesystem::path include_base = {};

  /// @brief Shared allocator context.
  Context ctx;
};

/// @brief Loads a CCCR schema from a JSON string.
/// @param json UTF-8 JSON text
/// @param params Optional include base and allocator
/// @return Parsed schema or a JsonError
[[nodiscard]] auto LoadSchema(std::string_view json,
                              const LoadSchemaParams& params = {})
    -> JsonResult<ast::Schema>;

}  // namespace cccr::json
