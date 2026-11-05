#pragma once

#include <cccr/context.hpp>

#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace cccr::rdf {

/// @brief IRI term.
struct Iri {
  /// @brief Returns an empty IRI that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty IRI bound to @p ctx
  [[nodiscard]] static Iri FromContext(cccr::Context ctx) noexcept {
    return {.value = std::pmr::string{ctx.GetAllocator()}};
  }

  /// @brief Returns an IRI from an already-allocated lexical form.
  /// @param value Absolute IRI lexical form
  /// @return IRI term
  [[nodiscard]] static Iri From(std::pmr::string value) noexcept {
    return {.value = std::move(value)};
  }

  /// @brief Returns an IRI copied from @p value using @p ctx.
  /// @param value Absolute IRI lexical form
  /// @param ctx Shared allocator context
  /// @return IRI term
  [[nodiscard]] static Iri From(std::string_view value,
                                cccr::Context ctx) noexcept {
    return {.value = std::pmr::string{value, ctx.GetAllocator()}};
  }

  /// @brief Absolute IRI lexical form.
  std::pmr::string value{std::pmr::get_default_resource()};
};

/// @brief Plain or typed literal term.
struct Literal {
  /// @brief Returns an empty literal that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty literal bound to @p ctx
  [[nodiscard]] static Literal FromContext(cccr::Context ctx) noexcept {
    return {
        .value = std::pmr::string{ctx.GetAllocator()},
        .datatype = std::nullopt,
        .language = std::nullopt,
    };
  }

  /// @brief Returns a literal from lexical form and optional annotations.
  /// @param value Lexical form
  /// @param datatype Optional datatype IRI
  /// @param language Optional language tag
  /// @return Literal term
  [[nodiscard]] static Literal From(
      std::pmr::string value,
      std::optional<std::pmr::string> datatype = std::nullopt,
      std::optional<std::pmr::string> language = std::nullopt) noexcept {
    return {
        .value = std::move(value),
        .datatype = std::move(datatype),
        .language = std::move(language),
    };
  }

  /// @brief Lexical form of the literal.
  std::pmr::string value{std::pmr::get_default_resource()};
  /// @brief Optional datatype IRI (mutually exclusive with @ref language in
  ///        RDF/XML serialization).
  std::optional<std::pmr::string> datatype;
  /// @brief Optional language tag (`xml:lang`).
  std::optional<std::pmr::string> language;
};

/// @brief Object of a triple (IRI or literal).
using Term = std::variant<Iri, Literal>;

/// @brief One RDF triple.
struct Triple {
  /// @brief Returns an empty triple that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty triple bound to @p ctx
  [[nodiscard]] static Triple FromContext(cccr::Context ctx) noexcept {
    return {
        .subject = Iri::FromContext(ctx),
        .predicate = Iri::FromContext(ctx),
        .object = Iri::FromContext(ctx),
    };
  }

  /// @brief Returns a triple from its three terms.
  /// @param subject Subject IRI
  /// @param predicate Predicate IRI
  /// @param object Object term
  /// @return Triple
  [[nodiscard]] static Triple From(Iri subject, Iri predicate,
                                   Term object) noexcept {
    return {
        .subject = std::move(subject),
        .predicate = std::move(predicate),
        .object = std::move(object),
    };
  }

  /// @brief Triple subject IRI.
  Iri subject;
  /// @brief Triple predicate IRI.
  Iri predicate;
  /// @brief Triple object (IRI or literal).
  Term object;
};

/// @brief Returns whether @p term is an IRI equal to @p iri.
/// @param term Object term
/// @param iri Expected IRI value
/// @return `true` on match
[[nodiscard]] constexpr bool TermIsIri(const Term& term,
                                       std::string_view iri) noexcept {
  const auto* value = std::get_if<Iri>(&term);
  return value != nullptr && value->value == iri;
}

/// @brief Returns whether @p term is a plain/typed literal equal to @p value.
/// @param term Object term
/// @param value Expected lexical form
/// @return `true` on match
[[nodiscard]] constexpr bool TermIsLiteral(const Term& term,
                                           std::string_view value) noexcept {
  const auto* lit = std::get_if<Literal>(&term);
  return lit != nullptr && lit->value == value;
}

}  // namespace cccr::rdf
