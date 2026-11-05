#pragma once

#include <glaze/glaze.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace cccr::json::detail {

/// Wire DTO for `{"include": "path"}` inside `xquery`.
struct XQueryIncludeDto {
  std::string include;
};

/// Wire DTO for xquery: string, [select, value] array, or include object.
using XQueryDto =
    std::variant<std::string, std::vector<std::string>, XQueryIncludeDto>;

/// Wire DTO for `{"xquery"|"sparql"|"include": "...", "context"?: {...}}`.
/// @details `context` uses `glz::generic` to avoid a recursive DTO cycle with
///          `ComputeWrapDto` / `ContextEntryDto`.
struct ComputeDto {
  std::optional<XQueryDto> xquery;
  std::optional<std::string> sparql;
  std::optional<std::string> include;
  std::optional<std::unordered_map<std::string, glz::generic>> context;
};

/// Wire DTO for `{"compute": {...}}`.
struct ComputeWrapDto {
  ComputeDto compute;
};

/// String literal or nested compute object (glaze auto-deduces by JSON type).
using ValueDto = std::variant<std::string, ComputeWrapDto>;

/// Context map entry: value expr, or a string array (predicates list).
using ContextEntryDto =
    std::variant<std::string, ComputeWrapDto, std::vector<std::string>>;

struct QNameDto {
  std::string name_space;
  std::string prefixed_name;
};

struct PropertyDto;

struct DescriptionDto {
  std::optional<std::unordered_map<std::string, ContextEntryDto>> context;
  std::optional<ValueDto> about;
  std::optional<std::vector<PropertyDto>> properties;
};

struct PropertyDto {
  std::optional<QNameDto> qname;
  std::optional<std::unordered_map<std::string, ContextEntryDto>> context;
  std::optional<ValueDto> uri;
  std::optional<ValueDto> literal;
  std::optional<std::string> datatype;
  std::optional<std::string> language;
  std::optional<std::string> min_inclusive;
  std::optional<std::string> max_inclusive;
  std::optional<std::vector<PropertyDto>> properties;
  std::optional<DescriptionDto> description;
  std::optional<std::string> include;
};

struct SchemaDto {
  DescriptionDto description;
};

/// Fragment file shapes for property-level include.
struct PropertiesWrapDto {
  std::optional<std::vector<PropertyDto>> properties;
};

}  // namespace cccr::json::detail

template <>
struct glz::meta<cccr::json::detail::XQueryIncludeDto> {
  using T = cccr::json::detail::XQueryIncludeDto;
  static constexpr auto value = glz::object("include", &T::include);
};

template <>
struct glz::meta<cccr::json::detail::QNameDto> {
  using T = cccr::json::detail::QNameDto;
  static constexpr auto value = glz::object(  //
      "NameSpace", &T::name_space,            //
      "PrefixedName", &T::prefixed_name);
};

template <>
struct glz::meta<cccr::json::detail::PropertyDto> {
  using T = cccr::json::detail::PropertyDto;
  static constexpr auto value = glz::object(  //
      "QName", &T::qname,                     //
      "context", &T::context,                 //
      "URI", &T::uri,                         //
      "literal", &T::literal,                 //
      "datatype", &T::datatype,               //
      "language", &T::language,               //
      "minInclusive", &T::min_inclusive,      //
      "maxInclusive", &T::max_inclusive,      //
      "properties", &T::properties,           //
      "description", &T::description,         //
      "include", &T::include);
};
