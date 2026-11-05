#pragma once

#include <cccr/ast/types.hpp>
#include <cccr/context.hpp>
#include <cccr/eval/error.hpp>
#include <cccr/input/source.hpp>
#include <cccr/rdf/graph.hpp>
#include <cccr/sparql/substitute.hpp>

#include <filesystem>
#include <string>

namespace cccr::eval {

/// @brief Optional parameters for @ref EvalToString / @ref EvalToNodes.
struct EvalParams {
  /// @brief Returns params from @p include_base and @p ctx.
  /// @param include_base Directory used to resolve relative `include` paths
  /// @param ctx Shared allocator context
  /// @return Populated params
  [[nodiscard]] static EvalParams From(std::filesystem::path include_base,
                                       Context ctx) noexcept {
    return {
        .include_base = std::move(include_base),
        .ctx = ctx,
    };
  }

  /// @brief Directory used to resolve relative `include` paths.
  std::filesystem::path include_base = {};

  /// @brief Shared allocator context.
  Context ctx;
};

/// @brief Evaluates a value expression to a string (XQuery, SPARQL, or
/// include).
/// @param expr Literal or compute expression
/// @param source Input document backend
/// @param context Current context node
/// @param knowledge SPARQL knowledge graph
/// @param pseudos Pseudo-variable bindings for `{{name}}`
/// @param params Optional include base and allocator
/// @return String value or EvalError
[[nodiscard]] auto EvalToString(const ast::ValueExpr& expr,
                                input::Source& source, input::NodeRef context,
                                const rdf::Graph& knowledge,
                                const sparql::PseudoMap& pseudos,
                                const EvalParams& params = {})
    -> EvalResult<std::pmr::string>;

/// @brief Evaluates a subject-selector expression to a node sequence.
/// @param expr Selector expression (`compute.xquery` select form)
/// @param source Input document backend
/// @param context Parent context node
/// @param params Optional include base and allocator
/// @return Selected nodes or EvalError
[[nodiscard]] auto EvalToNodes(const ast::ValueExpr& expr,
                               input::Source& source, input::NodeRef context,
                               const EvalParams& params = {})
    -> EvalResult<std::pmr::vector<input::NodeRef>>;

}  // namespace cccr::eval
