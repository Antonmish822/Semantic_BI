#pragma once

#include <cccr/context.hpp>
#include <cccr/json/error.hpp>

#include <json/dto.hpp>

#include <filesystem>
#include <vector>

namespace cccr::json {

/// @brief Optional parameters for @ref ExpandPropertyIncludes.
struct ExpandPropertyIncludesParams {
  std::filesystem::path include_base = {};
  Context ctx;
};

[[nodiscard]] auto ExpandPropertyIncludes(
    std::vector<detail::PropertyDto>& properties,
    const ExpandPropertyIncludesParams& params = {}) -> JsonResult<void>;

}  // namespace cccr::json
