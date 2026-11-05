#include <pch.hpp>

#include <cccr/eval/string.hpp>

#include <cccr/context.hpp>
#include <cccr/input/node.hpp>
#include <cccr/input/query.hpp>
#include <cccr/input/source.hpp>
#include <cccr/sparql/select.hpp>
#include <cccr/sparql/substitute.hpp>
#include <cccr/utils/filesystem.hpp>

#include <detail/path.hpp>

#include <expected>
#include <utility>

namespace cccr::eval {

namespace {

[[nodiscard]] auto EvalXQuery(const ast::Compute& compute,
                              input::Source& source, input::NodeRef context,
                              const EvalParams& params, Context ctx)
    -> EvalResult<std::pmr::string> {
  std::pmr::string expression{compute.expression, ctx.GetAllocator()};
  if (compute.xquery_include) {
    const auto path =
        detail::ResolveIncludePath(compute.expression, params.include_base);
    auto contents = utils::ReadFileToString(path, ctx);
    if (!contents) [[unlikely]] {
      return std::unexpected(EvalError::kIncludeFailed);
    }
    expression = std::move(*contents);
  }

  if (!compute.expression2) {
    auto result = source.EvalString(context, input::Query::From(expression));
    if (!result) [[unlikely]] {
      return std::unexpected(EvalError::kXQueryFailed);
    }
    return std::move(*result);
  }

  // Dual form: expr[0] selects nodes; expr[1] string-value on each (joined).
  if (compute.xquery_include) [[unlikely]] {
    return std::unexpected(EvalError::kUnsupported);
  }

  auto nodes =
      source.EvalNodes(context, input::Query::From(compute.expression));
  if (!nodes) [[unlikely]] {
    return std::unexpected(EvalError::kXQueryFailed);
  }

  std::pmr::string out{ctx.GetAllocator()};
  for (const input::NodeRef node : *nodes) {
    auto piece =
        source.EvalString(node, input::Query::From(*compute.expression2));
    if (!piece) [[unlikely]] {
      return std::unexpected(EvalError::kXQueryFailed);
    }

    if (piece->empty()) {
      continue;
    }

    if (!out.empty()) {
      out.push_back(' ');
    }
    out += *piece;
  }

  return out;
}

[[nodiscard]] sparql::PseudoMap CopyPseudoMap(const sparql::PseudoMap& source,
                                              Context ctx) {
  sparql::PseudoMap out{ctx.GetAllocator()};
  out.reserve(source.size());
  for (const auto& [name, value] : source) {
    out.emplace(std::pmr::string{name, ctx.GetAllocator()},
                std::pmr::string{value, ctx.GetAllocator()});
  }
  return out;
}

}  // namespace

auto EvalToString(const ast::ValueExpr& expr, input::Source& source,
                  input::NodeRef context, const rdf::Graph& knowledge,
                  const sparql::PseudoMap& pseudos, const EvalParams& params)
    -> EvalResult<std::pmr::string> {
  const Context ctx = params.ctx;
  auto* const mr = ctx.GetAllocator();

  if (expr.literal) {
    return std::pmr::string{*expr.literal, mr};
  }

  if (!expr.compute) [[unlikely]] {
    return std::unexpected(EvalError::kMissingValue);
  }

  const auto& compute = *expr.compute;
  switch (compute.kind) {
    using enum ast::EvaluatorKind;
    case kXQuery:
      return EvalXQuery(compute, source, context, params, ctx);
    case kSparql: {
      auto local = CopyPseudoMap(pseudos, ctx);
      for (const auto& [name, binding] : expr.sparql_pseudos) {
        auto value =
            EvalToString(binding, source, context, knowledge, local, params);
        if (!value) [[unlikely]] {
          return value;
        }
        local.insert_or_assign(std::pmr::string{name, mr}, std::move(*value));
      }

      const auto query =
          sparql::SubstitutePseudos(compute.expression, local, ctx);
      auto result = sparql::SelectFirst(query, knowledge, ctx);
      if (!result) [[unlikely]] {
        return std::unexpected(EvalError::kSparqlFailed);
      }
      return std::move(*result);
    }
    case kInclude: {
      const auto path =
          detail::ResolveIncludePath(compute.expression, params.include_base);
      auto contents = utils::ReadFileToString(path, ctx);
      if (!contents) [[unlikely]] {
        return std::unexpected(EvalError::kIncludeFailed);
      }
      return std::move(*contents);
    }
    default:
      return std::unexpected(EvalError::kUnsupported);
  }
}

auto EvalToNodes(const ast::ValueExpr& expr, input::Source& source,
                 input::NodeRef context, const EvalParams& params)
    -> EvalResult<std::pmr::vector<input::NodeRef>> {
  const Context ctx = params.ctx;

  if (!expr.compute || expr.compute->kind != ast::EvaluatorKind::kXQuery)
      [[unlikely]] {
    return std::unexpected(EvalError::kUnsupported);
  }

  std::pmr::string expression{expr.compute->expression, ctx.GetAllocator()};
  if (expr.compute->xquery_include) {
    const auto path = detail::ResolveIncludePath(expr.compute->expression,
                                                 params.include_base);
    auto contents = utils::ReadFileToString(path, ctx);
    if (!contents) [[unlikely]] {
      return std::unexpected(EvalError::kIncludeFailed);
    }
    expression = std::move(*contents);
  }

  // Subject selection uses the first expression only (dual-form select).
  auto nodes = source.EvalNodes(context, input::Query::From(expression));
  if (!nodes) [[unlikely]] {
    return std::unexpected(EvalError::kXQueryFailed);
  }
  return std::move(*nodes);
}

}  // namespace cccr::eval
