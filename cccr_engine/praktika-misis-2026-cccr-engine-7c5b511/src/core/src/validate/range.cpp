#include <pch.hpp>

#include <cccr/validate/range.hpp>

#include <cccr/context.hpp>
#include <cccr/rdf/term.hpp>
#include <cccr/validate/literal.hpp>

#include <cstddef>
#include <expected>
#include <functional>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace cccr::validate {

namespace {

constexpr std::string_view kRdfsRange =
    "http://www.w3.org/2000/01/rdf-schema#range";
constexpr std::string_view kRdfType =
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#type";

struct TypeKey {
  std::string_view instance;
  std::string_view type;

  [[nodiscard]] friend bool operator==(const TypeKey& lhs,
                                       const TypeKey& rhs) noexcept {
    return lhs.instance == rhs.instance && lhs.type == rhs.type;
  }
};

struct TypeKeyHash {
  [[nodiscard]] size_t operator()(const TypeKey& key) const noexcept {
    return std::hash<std::string_view>{}(key.instance) ^
           (std::hash<std::string_view>{}(key.type) << 1);
  }
};

using RangeMap = std::pmr::unordered_map<std::string_view,
                                         std::pmr::vector<std::string_view>>;
using TypeSet = std::pmr::unordered_set<TypeKey, TypeKeyHash>;

[[nodiscard]] auto CollectRanges(const rdf::Graph& ontology,
                                 std::pmr::memory_resource* mr) -> RangeMap {
  RangeMap ranges{mr};
  for (const auto& triple : ontology.Triples()) {
    if (triple.predicate.value != kRdfsRange) {
      continue;
    }
    const auto* range_iri = std::get_if<rdf::Iri>(&triple.object);
    if (range_iri == nullptr) {
      continue;
    }
    ranges[std::string_view{triple.subject.value}].push_back(
        std::string_view{range_iri->value});
  }
  return ranges;
}

[[nodiscard]] auto CollectTypeSet(const rdf::Graph& data,
                                  const rdf::Graph& ontology,
                                  std::pmr::memory_resource* mr) -> TypeSet {
  TypeSet types{mr};
  auto absorb = [&](const rdf::Graph& graph) {
    for (const auto& triple : graph.Triples()) {
      if (triple.predicate.value != kRdfType) {
        continue;
      }
      const auto* type_iri = std::get_if<rdf::Iri>(&triple.object);
      if (type_iri == nullptr) {
        continue;
      }
      types.insert(TypeKey{
          .instance = std::string_view{triple.subject.value},
          .type = std::string_view{type_iri->value},
      });
    }
  };

  absorb(data);
  absorb(ontology);
  return types;
}

[[nodiscard]] auto CheckOneRange(const rdf::Term& object,
                                 std::string_view range, const TypeSet& types)
    -> ValidateResult<void> {
  if (const auto* lit = std::get_if<rdf::Literal>(&object)) {
    if (!KnownXsdDatatype(range)) {
      return std::unexpected(ValidateError::kRangeViolation);
    }
    return CheckLiteral(lit->value, range, std::nullopt, std::nullopt);
  }

  const auto& iri = std::get<rdf::Iri>(object);
  if (KnownXsdDatatype(range)) {
    return std::unexpected(ValidateError::kRangeViolation);
  }

  if (!types.contains(
          TypeKey{.instance = std::string_view{iri.value}, .type = range})) {
    return std::unexpected(ValidateError::kRangeViolation);
  }
  return {};
}

}  // namespace

auto CheckRdfsRange(const rdf::Graph& data, const rdf::Graph& ontology,
                    Context ctx) -> ValidateResult<void> {
  auto* const mr = ctx.GetAllocator();
  const auto ranges = CollectRanges(ontology, mr);
  if (ranges.empty()) {
    return {};
  }

  const auto types = CollectTypeSet(data, ontology, mr);

  for (const auto& triple : data.Triples()) {
    const auto it = ranges.find(std::string_view{triple.predicate.value});
    if (it == ranges.end()) {
      continue;
    }
    for (const auto range : it->second) {
      auto checked = CheckOneRange(triple.object, range, types);
      if (!checked) [[unlikely]] {
        return std::unexpected(checked.error());
      }
    }
  }

  return {};
}

}  // namespace cccr::validate
