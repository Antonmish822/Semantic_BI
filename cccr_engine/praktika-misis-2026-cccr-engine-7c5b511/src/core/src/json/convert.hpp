#pragma once

#include <cccr/ast/types.hpp>
#include <cccr/context.hpp>
#include <cccr/json/error.hpp>

#include <json/dto.hpp>

namespace cccr::json {

[[nodiscard]] auto ToSchema(detail::SchemaDto&& dto, Context ctx = {})
    -> JsonResult<ast::Schema>;

}  // namespace cccr::json
