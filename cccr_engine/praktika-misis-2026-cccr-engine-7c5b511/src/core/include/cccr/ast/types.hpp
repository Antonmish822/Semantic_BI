#pragma once

#include <cccr/context.hpp>

#include <cstdint>
#include <memory_resource>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cccr::ast {

/// @brief Evaluator kind inside a `compute` block.
enum class EvaluatorKind : uint8_t {
  /// @brief XQuery over the input document.
  kXQuery,
  /// @brief SPARQL over the knowledge / working graph.
  kSparql,
  /// @brief Load a filesystem path as a string value.
  kInclude,
};

/// @brief A `compute` expression (`xquery`, `sparql`, or `include`).
/// @details For `kXQuery`, @ref expression2 may hold the second expression of a
///          two-element `xquery` array (select + string-value). When
///          @ref xquery_include is true, @ref expression is a filesystem path
///          whose contents are loaded and run as XQuery.
struct Compute {
  /// @brief Returns an empty compute that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty compute bound to @p ctx
  [[nodiscard]] static Compute FromContext(cccr::Context ctx) noexcept {
    return {
        .kind = {},
        .expression = std::pmr::string{ctx.GetAllocator()},
        .expression2 = std::nullopt,
        .xquery_include = false,
    };
  }

  /// @brief Which evaluator interprets @ref expression.
  EvaluatorKind kind{};
  /// @brief Primary expression text, SPARQL query, or include path.
  std::pmr::string expression{std::pmr::get_default_resource()};
  /// @brief Optional second XQuery expression (dual-form string-value).
  std::optional<std::pmr::string> expression2;
  /// @brief When `true`, @ref expression is an include path for XQuery text.
  bool xquery_include = false;
};

/// @brief Plain string or a nested `compute` block.
/// @details @ref sparql_pseudos holds optional local pseudo-variables for a
///          SPARQL `compute` (grammar: optional Context on sparql).
struct ValueExpr {
  /// @brief Returns an empty value expression that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty value expression bound to @p ctx
  [[nodiscard]] static ValueExpr FromContext(cccr::Context ctx) noexcept {
    return {
        .literal = std::nullopt,
        .compute = std::nullopt,
        .sparql_pseudos =
            std::pmr::vector<std::pair<std::pmr::string, ValueExpr>>{
                ctx.GetAllocator()},
    };
  }

  /// @brief Static string value when no compute is present.
  std::optional<std::pmr::string> literal;
  /// @brief Dynamic value produced by an evaluator.
  std::optional<Compute> compute;
  /// @brief Local pseudo bindings for SPARQL (`name` → expression).
  std::pmr::vector<std::pair<std::pmr::string, ValueExpr>> sparql_pseudos{
      std::pmr::get_default_resource()};
};

/// @brief Qualified property name from the schema.
struct QName {
  /// @brief Returns an empty QName that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty QName bound to @p ctx
  [[nodiscard]] static QName FromContext(cccr::Context ctx) noexcept {
    return {
        .name_space = std::pmr::string{ctx.GetAllocator()},
        .prefixed_name = std::pmr::string{ctx.GetAllocator()},
    };
  }

  /// @brief Namespace IRI base for the property.
  std::pmr::string name_space{std::pmr::get_default_resource()};
  /// @brief Prefixed local name (e.g. `rdf:type`); prefix is stripped at emit.
  std::pmr::string prefixed_name{std::pmr::get_default_resource()};
};

/// @brief Predicate list: static IRIs and/or a compute that expands to IRIs.
struct PredicatesList {
  /// @brief Returns an empty list that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty predicate list bound to @p ctx
  [[nodiscard]] static PredicatesList FromContext(cccr::Context ctx) noexcept {
    return {
        .values = std::pmr::vector<std::pmr::string>{ctx.GetAllocator()},
        .compute = std::nullopt,
    };
  }

  /// @brief Static predicate IRI strings.
  std::pmr::vector<std::pmr::string> values{std::pmr::get_default_resource()};
  /// @brief Optional compute that expands to whitespace-separated IRIs.
  std::optional<ValueExpr> compute;
};

/// @brief Local execution state: subject selector and/or pseudo-variables.
struct Context {
  /// @brief Returns an empty context that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty AST context bound to @p ctx
  [[nodiscard]] static Context FromContext(cccr::Context ctx) noexcept {
    return {
        .subject = std::nullopt,
        .pseudo_variables =
            std::pmr::unordered_map<std::pmr::string, ValueExpr>{
                ctx.GetAllocator()},
        .predicates = std::nullopt,
    };
  }

  /// @brief Optional subject-node selector (`compute.xquery`).
  std::optional<ValueExpr> subject;
  /// @brief Named pseudo-variables available to nested computes.
  std::pmr::unordered_map<std::pmr::string, ValueExpr> pseudo_variables{
      std::pmr::get_default_resource()};
  /// @brief Optional default predicate fan-out for child properties.
  std::optional<PredicatesList> predicates;
};

/// @brief Optional datatype / language / range checks for a literal property.
struct LiteralConstraint {
  /// @brief XSD datatype local name or IRI for typed literals.
  std::optional<std::pmr::string> datatype;
  /// @brief Language tag (`xml:lang`) when no datatype is set.
  std::optional<std::pmr::string> language;
  /// @brief Inclusive lower bound (lexical / numeric per datatype).
  std::optional<std::pmr::string> min_inclusive;
  /// @brief Inclusive upper bound (lexical / numeric per datatype).
  std::optional<std::pmr::string> max_inclusive;
};

struct Property;

/// @brief A `description` block (`context` / `about` / `properties`).
struct Description {
  /// @brief Returns an empty description that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty description bound to @p ctx
  [[nodiscard]] static Description FromContext(cccr::Context ctx) noexcept;

  /// @brief Optional local context (subject, pseudos, predicates).
  std::optional<Context> context;
  /// @brief Subject IRI (`rdf:about`) as literal or compute.
  std::optional<ValueExpr> about;
  /// @brief Property entries under this subject.
  std::pmr::vector<Property> properties{std::pmr::get_default_resource()};
};

/// @brief One property entry (URI/literal, nested group, or nested
/// Description).
struct Property {
  /// @brief Returns an empty property that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty property bound to @p ctx
  [[nodiscard]] static Property FromContext(cccr::Context ctx) noexcept;

  /// @brief Predicate QName when not using a predicates list.
  std::optional<QName> qname;
  /// @brief Optional property-local context (pseudos / predicates).
  std::optional<Context> context;
  /// @brief Object as a resource IRI (static or compute).
  std::optional<ValueExpr> uri;
  /// @brief Object as a literal (static or compute).
  std::optional<ValueExpr> literal;
  /// @brief Optional datatype / language / range for literal emission.
  std::optional<LiteralConstraint> constraint;
  /// @brief Nested property group sharing the same subject.
  std::pmr::vector<Property> properties{std::pmr::get_default_resource()};
  /// @brief Nested Description object (new subject + link triple).
  std::optional<Description> description;
};

inline Description Description::FromContext(cccr::Context ctx) noexcept {
  return {
      .context = std::nullopt,
      .about = std::nullopt,
      .properties = std::pmr::vector<Property>{ctx.GetAllocator()},
  };
}

inline Property Property::FromContext(cccr::Context ctx) noexcept {
  return {
      .qname = std::nullopt,
      .context = std::nullopt,
      .uri = std::nullopt,
      .literal = std::nullopt,
      .constraint = std::nullopt,
      .properties = std::pmr::vector<Property>{ctx.GetAllocator()},
      .description = std::nullopt,
  };
}

/// @brief Root CCCR schema: `{"description": {...}}`.
struct Schema {
  /// @brief Returns an empty schema that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty schema bound to @p ctx
  [[nodiscard]] static Schema FromContext(cccr::Context ctx) noexcept {
    return {.description = Description::FromContext(ctx)};
  }

  /// @brief Top-level description block.
  Description description;
};

}  // namespace cccr::ast
