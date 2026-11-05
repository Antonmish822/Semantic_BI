#include <pch.hpp>

#include <json/convert.hpp>

#include <cccr/context.hpp>
#include <cccr/log/log.hpp>

#include <glaze/glaze.hpp>

#include <cstddef>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace cccr::json {

namespace {

using detail::ComputeDto;
using detail::ComputeWrapDto;
using detail::ContextEntryDto;
using detail::DescriptionDto;
using detail::PropertyDto;
using detail::QNameDto;
using detail::SchemaDto;
using detail::ValueDto;
using detail::XQueryDto;

[[nodiscard]] std::pmr::string ToPmr(std::string_view text, Context ctx) {
  return std::pmr::string{text, ctx.GetAllocator()};
}

[[nodiscard]] auto ToPmrOpt(const std::optional<std::string>& text, Context ctx)
    -> std::optional<std::pmr::string> {
  if (!text) [[unlikely]] {
    return std::nullopt;
  }

  return ToPmr(*text, ctx);
}

struct PropertyConvertFrame {
  std::reference_wrapper<const PropertyDto> dto;
  std::reference_wrapper<ast::Property> out;
};

[[nodiscard]] auto ToCompute(const ComputeDto& dto, Context ctx)
    -> JsonResult<ast::Compute> {
  const int count = static_cast<int>(dto.xquery.has_value()) +
                    static_cast<int>(dto.sparql.has_value()) +
                    static_cast<int>(dto.include.has_value());
  if (count != 1) [[unlikely]] {
    log::Error(ctx,
               "Invalid compute: expected exactly one of xquery, sparql, or "
               "include (got {})",
               count);
    return std::unexpected(JsonError::kInvalidSchema);
  }

  if (dto.context && !dto.sparql) [[unlikely]] {
    log::Error(ctx, "Invalid compute: context is only allowed with sparql");
    return std::unexpected(JsonError::kInvalidSchema);
  }

  if (dto.xquery) {
    if (const auto* single = std::get_if<std::string>(&*dto.xquery)) {
      return ast::Compute{
          .kind = ast::EvaluatorKind::kXQuery,
          .expression = ToPmr(*single, ctx),
          .expression2 = std::nullopt,
          .xquery_include = false,
      };
    }

    if (const auto* included =
            std::get_if<detail::XQueryIncludeDto>(&*dto.xquery)) {
      if (included->include.empty()) [[unlikely]] {
        log::Error(ctx, "Invalid xquery include: path is empty");
        return std::unexpected(JsonError::kInvalidSchema);
      }
      return ast::Compute{
          .kind = ast::EvaluatorKind::kXQuery,
          .expression = ToPmr(included->include, ctx),
          .expression2 = std::nullopt,
          .xquery_include = true,
      };
    }

    const auto& parts = std::get<std::vector<std::string>>(*dto.xquery);
    if (parts.size() != 2 || parts[0].empty() || parts[1].empty())
        [[unlikely]] {
      log::Error(
          ctx,
          "Invalid dual xquery: expected two non-empty strings (got {} parts)",
          parts.size());
      return std::unexpected(JsonError::kInvalidSchema);
    }

    return ast::Compute{
        .kind = ast::EvaluatorKind::kXQuery,
        .expression = ToPmr(parts[0], ctx),
        .expression2 = ToPmr(parts[1], ctx),
        .xquery_include = false,
    };
  }

  if (dto.sparql) {
    return ast::Compute{
        .kind = ast::EvaluatorKind::kSparql,
        .expression = ToPmr(*dto.sparql, ctx),
        .expression2 = std::nullopt,
        .xquery_include = false,
    };
  }

  return ast::Compute{
      .kind = ast::EvaluatorKind::kInclude,
      .expression = ToPmr(*dto.include, ctx),
      .expression2 = std::nullopt,
      .xquery_include = false,
  };
}

[[nodiscard]] auto ToValueExprFromGeneric(const glz::generic& value,
                                          Context ctx)
    -> JsonResult<ast::ValueExpr>;

[[nodiscard]] auto ToSparqlPseudos(
    const std::unordered_map<std::string, glz::generic>& context, Context ctx)
    -> JsonResult<
        std::pmr::vector<std::pair<std::pmr::string, ast::ValueExpr>>> {
  std::pmr::vector<std::pair<std::pmr::string, ast::ValueExpr>> out{
      ctx.GetAllocator()};
  out.reserve(context.size());
  for (const auto& [name, value] : context) {
    if (name == "subject" || name == "predicates") [[unlikely]] {
      log::Error(ctx,
                 "Invalid sparql compute.context: reserved name \"{}\" is not "
                 "allowed",
                 name);
      return std::unexpected(JsonError::kInvalidSchema);
    }

    auto expr = ToValueExprFromGeneric(value, ctx);
    if (!expr) [[unlikely]] {
      return std::unexpected(expr.error());
    }

    out.emplace_back(ToPmr(name, ctx), std::move(*expr));
  }

  return out;
}

[[nodiscard]] auto ToValueExpr(const ValueDto& dto, Context ctx)
    -> JsonResult<ast::ValueExpr> {
  if (const auto* literal = std::get_if<std::string>(&dto)) {
    return ast::ValueExpr{
        .literal = ToPmr(*literal, ctx),
        .compute = std::nullopt,
        .sparql_pseudos =
            std::pmr::vector<std::pair<std::pmr::string, ast::ValueExpr>>{
                ctx.GetAllocator()},
    };
  }

  const auto& wrap = std::get<ComputeWrapDto>(dto);
  auto compute = ToCompute(wrap.compute, ctx);
  if (!compute) [[unlikely]] {
    return std::unexpected(compute.error());
  }

  std::pmr::vector<std::pair<std::pmr::string, ast::ValueExpr>> sparql_pseudos{
      ctx.GetAllocator()};
  if (wrap.compute.context) {
    auto pseudos = ToSparqlPseudos(*wrap.compute.context, ctx);
    if (!pseudos) [[unlikely]] {
      return std::unexpected(pseudos.error());
    }
    sparql_pseudos = std::move(*pseudos);
  }

  return ast::ValueExpr{
      .literal = std::nullopt,
      .compute = std::move(*compute),
      .sparql_pseudos = std::move(sparql_pseudos),
  };
}

[[nodiscard]] auto ToValueExprFromGeneric(const glz::generic& value,
                                          Context ctx)
    -> JsonResult<ast::ValueExpr> {
  std::string json;
  const auto write_err = glz::write_json(value, json);
  if (write_err) [[unlikely]] {
    log::Error(ctx,
               "Invalid value expression: failed to serialize generic JSON");
    return std::unexpected(JsonError::kInvalidSchema);
  }

  ValueDto dto{};
  const auto read_err = glz::read_json(dto, json);
  if (read_err) [[unlikely]] {
    log::Error(ctx, "Invalid value expression: {}",
               glz::format_error(read_err, json));
    return std::unexpected(JsonError::kInvalidSchema);
  }

  return ToValueExpr(dto, ctx);
}

[[nodiscard]] auto ToValueExprFromEntry(const ContextEntryDto& dto, Context ctx)
    -> JsonResult<ast::ValueExpr> {
  if (std::holds_alternative<std::vector<std::string>>(dto)) [[unlikely]] {
    log::Error(
        ctx,
        "Invalid context entry: string list is only allowed for predicates");
    return std::unexpected(JsonError::kInvalidSchema);
  }

  if (const auto* literal = std::get_if<std::string>(&dto)) {
    return ast::ValueExpr{
        .literal = ToPmr(*literal, ctx),
        .compute = std::nullopt,
        .sparql_pseudos =
            std::pmr::vector<std::pair<std::pmr::string, ast::ValueExpr>>{
                ctx.GetAllocator()},
    };
  }

  return ToValueExpr(ValueDto{std::get<ComputeWrapDto>(dto)}, ctx);
}

[[nodiscard]] auto ToContext(
    const std::unordered_map<std::string, ContextEntryDto>& dto, Context ctx)
    -> JsonResult<ast::Context> {
  auto context = ast::Context::FromContext(ctx);
  for (const auto& [key, value] : dto) {
    if (key == "predicates") {
      auto list = ast::PredicatesList::FromContext(ctx);
      if (const auto* values = std::get_if<std::vector<std::string>>(&value)) {
        list.values.reserve(values->size());
        for (const auto& item : *values) {
          list.values.push_back(ToPmr(item, ctx));
        }
      } else {
        auto expr = ToValueExprFromEntry(value, ctx);
        if (!expr) [[unlikely]] {
          return std::unexpected(expr.error());
        }
        list.compute = std::move(*expr);
      }
      context.predicates = std::move(list);
      continue;
    }

    auto expr = ToValueExprFromEntry(value, ctx);
    if (!expr) [[unlikely]] {
      return std::unexpected(expr.error());
    }

    if (key == "subject") {
      context.subject = std::move(*expr);
    } else {
      context.pseudo_variables.emplace(ToPmr(key, ctx), std::move(*expr));
    }
  }

  return context;
}

[[nodiscard]] auto FillPropertyScalars(const PropertyDto& dto,
                                       ast::Property& property, Context ctx)
    -> JsonResult<void> {
  if (dto.qname) {
    property.qname = ast::QName{
        .name_space = ToPmr(dto.qname->name_space, ctx),
        .prefixed_name = ToPmr(dto.qname->prefixed_name, ctx),
    };
  }

  if (dto.context) {
    auto context = ToContext(*dto.context, ctx);
    if (!context) [[unlikely]] {
      return std::unexpected(context.error());
    }
    property.context = std::move(*context);
  }

  if (dto.uri) {
    auto uri = ToValueExpr(*dto.uri, ctx);
    if (!uri) [[unlikely]] {
      return std::unexpected(uri.error());
    }
    property.uri = std::move(*uri);
  }

  if (dto.literal) {
    auto literal = ToValueExpr(*dto.literal, ctx);
    if (!literal) [[unlikely]] {
      return std::unexpected(literal.error());
    }
    property.literal = std::move(*literal);
  }

  if (dto.datatype || dto.language || dto.min_inclusive || dto.max_inclusive) {
    property.constraint = ast::LiteralConstraint{
        .datatype = ToPmrOpt(dto.datatype, ctx),
        .language = ToPmrOpt(dto.language, ctx),
        .min_inclusive = ToPmrOpt(dto.min_inclusive, ctx),
        .max_inclusive = ToPmrOpt(dto.max_inclusive, ctx),
    };
  }

  return {};
}

[[nodiscard]] auto ToDescription(const DescriptionDto& dto, Context ctx)
    -> JsonResult<ast::Description>;

[[nodiscard]] auto ToProperties(const std::vector<PropertyDto>& dtos,
                                Context ctx)
    -> JsonResult<std::pmr::vector<ast::Property>> {
  std::pmr::vector<ast::Property> roots{ctx.GetAllocator()};
  roots.reserve(dtos.size());
  for (size_t i = 0; i < dtos.size(); ++i) {
    roots.push_back(ast::Property::FromContext(ctx));
  }

  std::pmr::vector<PropertyConvertFrame> stack{ctx.GetAllocator()};
  stack.reserve(dtos.size());
  for (size_t i = 0; i < dtos.size(); ++i) {
    stack.push_back(PropertyConvertFrame{.dto = dtos[i], .out = roots[i]});
  }

  while (!stack.empty()) {
    const auto frame = stack.back();
    stack.pop_back();

    auto filled = FillPropertyScalars(frame.dto.get(), frame.out.get(), ctx);
    if (!filled) [[unlikely]] {
      return std::unexpected(filled.error());
    }

    if (frame.dto.get().description) {
      auto nested = ToDescription(*frame.dto.get().description, ctx);
      if (!nested) [[unlikely]] {
        return std::unexpected(nested.error());
      }
      frame.out.get().description = std::move(*nested);
    }

    if (!frame.dto.get().properties || frame.dto.get().properties->empty()) {
      continue;
    }

    const auto& children = *frame.dto.get().properties;
    auto& out_children = frame.out.get().properties;
    out_children = std::pmr::vector<ast::Property>{ctx.GetAllocator()};
    out_children.reserve(children.size());
    for (size_t i = 0; i < children.size(); ++i) {
      out_children.push_back(ast::Property::FromContext(ctx));
    }

    for (size_t i = children.size(); i-- > 0;) {
      stack.push_back(
          PropertyConvertFrame{.dto = children[i], .out = out_children[i]});
    }
  }

  return roots;
}

[[nodiscard]] auto ToDescription(const DescriptionDto& dto, Context ctx)
    -> JsonResult<ast::Description> {
  std::optional<ast::Context> context;
  if (dto.context) {
    auto parsed = ToContext(*dto.context, ctx);
    if (!parsed) [[unlikely]] {
      return std::unexpected(parsed.error());
    }
    context = std::move(*parsed);
  }

  std::optional<ast::ValueExpr> about;
  if (dto.about) {
    auto parsed = ToValueExpr(*dto.about, ctx);
    if (!parsed) [[unlikely]] {
      return std::unexpected(parsed.error());
    }
    about = std::move(*parsed);
  }

  std::pmr::vector<ast::Property> properties{ctx.GetAllocator()};
  if (dto.properties) {
    auto parsed = ToProperties(*dto.properties, ctx);
    if (!parsed) [[unlikely]] {
      return std::unexpected(parsed.error());
    }
    properties = std::move(*parsed);
  }

  return ast::Description{
      .context = std::move(context),
      .about = std::move(about),
      .properties = std::move(properties),
  };
}

}  // namespace

auto ToSchema(detail::SchemaDto&& dto, Context ctx) -> JsonResult<ast::Schema> {
  auto description = ToDescription(dto.description, ctx);
  if (!description) [[unlikely]] {
    return std::unexpected(description.error());
  }

  return ast::Schema{.description = std::move(*description)};
}

}  // namespace cccr::json
