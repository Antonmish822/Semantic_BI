#include <pch.hpp>

#include <cccr/json/load.hpp>

#include <cccr/log/log.hpp>

#include <json/convert.hpp>
#include <json/dto.hpp>
#include <json/include_expand.hpp>

#include <glaze/glaze.hpp>

#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace cccr::json {

namespace {

constexpr glz::opts kReadOpts{.null_terminated = false};

}  // namespace

auto LoadSchema(std::string_view json, const LoadSchemaParams& params)
    -> JsonResult<ast::Schema> {
  detail::SchemaDto dto{};
  if (const auto ec = glz::read<kReadOpts>(dto, json)) [[unlikely]] {
    const std::string detail = glz::format_error(ec, json);
    log::Error(params.ctx, "JSON schema parse failed: {}", detail);
    return std::unexpected(JsonError::kParseFailed);
  }

  if (dto.description.properties) {
    auto expanded = ExpandPropertyIncludes(
        *dto.description.properties,
        {.include_base = params.include_base, .ctx = params.ctx});
    if (!expanded) [[unlikely]] {
      return std::unexpected(expanded.error());
    }
  }

  return ToSchema(std::move(dto), params.ctx);
}

}  // namespace cccr::json
