#pragma once

#include <cccr/rdf/term.hpp>

#include <redland.h>

#include <cstdint>
#include <expected>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <utility>

namespace cccr::detail {

/// @brief Errors from @ref RedlandStore::SelectFirst.
enum class RedlandQueryError : uint8_t {
  kFailed,
  kNoResult,
};

/// @brief Owns a librdf world + memory storage + model.
/// @details Move-only. Holds a PMR resource for temporary CCCR-side strings
///          (null-terminated C API copies). Redland's own heap is separate.
///          Failures are signaled via `bool` / `std::optional` /
///          `std::expected`; Redland does not throw.
class RedlandStore {
public:
  RedlandStore() = delete;
  RedlandStore(const RedlandStore&) = delete;
  RedlandStore(RedlandStore&& other) noexcept;
  ~RedlandStore() noexcept { Reset(); }

  RedlandStore& operator=(const RedlandStore&) = delete;
  RedlandStore& operator=(RedlandStore&& other) noexcept;

  /// @brief Creates an opened world with an empty memory model.
  /// @param allocator PMR resource for temporary string copies
  /// @return Store, or `std::nullopt` when Redland allocation fails
  [[nodiscard]] static auto Create(
      std::pmr::memory_resource* allocator = std::pmr::get_default_resource())
      -> std::optional<RedlandStore>;

  /// @brief Adds one CCCR triple to the model.
  /// @param triple Triple to store
  /// @return `true` on success
  [[nodiscard]] bool Add(const rdf::Triple& triple);

  /// @brief Parses RDF text into this model using a raptor parser name.
  /// @param text RDF document
  /// @param parser_name Redland/raptor name (`ntriples`, `turtle`, `rdfxml`, …)
  /// @return `true` on success
  [[nodiscard]] bool Parse(std::string_view text, std::string_view parser_name);

  /// @brief Parses N-Triples text into this model.
  /// @param ntriples N-Triples document
  /// @return `true` on success
  [[nodiscard]] bool ParseNTriples(std::string_view ntriples) {
    return Parse(ntriples, "ntriples");
  }

  /// @brief Serializes the model to RDF/XML.
  /// @param allocator PMR resource for the returned string; defaults to
  ///                  @ref Allocator
  /// @return RDF/XML bytes, or `std::nullopt` on failure
  [[nodiscard]] auto SerializeRdfXml(
      std::pmr::memory_resource* allocator = nullptr) const
      -> std::optional<std::pmr::string>;

  /// @brief Runs a SPARQL SELECT and returns the first binding of the first
  ///        projected variable as a lexical string.
  /// @param query SPARQL query text
  /// @param allocator PMR resource for the returned string; defaults to
  ///                  @ref Allocator
  /// @return Lexical binding, or @ref RedlandQueryError
  [[nodiscard]] auto SelectFirst(std::string_view query,
                                 std::pmr::memory_resource* allocator = nullptr)
      const -> std::expected<std::pmr::string, RedlandQueryError>;

  /// @brief Runs a SPARQL query and serializes all results in @p format_name.
  /// @param query SPARQL query text
  /// @param format_name Rasqal formatter name (`json`, `xml`, `html`, …)
  /// @param allocator PMR resource for the returned string; defaults to
  ///                  @ref Allocator
  /// @return Formatted document, or @ref RedlandQueryError
  [[nodiscard]] auto QueryFormatted(
      std::string_view query, std::string_view format_name,
      std::pmr::memory_resource* allocator = nullptr) const
      -> std::expected<std::pmr::string, RedlandQueryError>;

  /// @brief Materializes all model statements as CCCR triples.
  /// @param allocator PMR resource for term strings; defaults to @ref Allocator
  /// @return Triples in model order, or `std::nullopt` on stream failure
  [[nodiscard]] auto MaterializeTriples(
      std::pmr::memory_resource* allocator = nullptr) const
      -> std::optional<std::pmr::vector<rdf::Triple>>;

  /// @brief Returns the PMR resource used for temporary allocations.
  /// @return Non-null memory resource
  [[nodiscard]] std::pmr::memory_resource* Allocator() const noexcept {
    return allocator_;
  }

  /// @brief Returns the librdf world.
  /// @return Non-null world pointer
  [[nodiscard]] librdf_world* World() const noexcept { return world_; }

  /// @brief Returns the librdf model.
  /// @return Non-null model pointer
  [[nodiscard]] librdf_model* Model() const noexcept { return model_; }

private:
  RedlandStore(librdf_world* world, librdf_storage* storage,
               librdf_model* model,
               std::pmr::memory_resource* allocator) noexcept;

  void Reset() noexcept;

  [[nodiscard]] auto ResolveAllocator(std::pmr::memory_resource* allocator)
      const noexcept -> std::pmr::memory_resource* {
    return allocator != nullptr ? allocator : allocator_;
  }

  librdf_world* world_ = nullptr;
  librdf_storage* storage_ = nullptr;
  librdf_model* model_ = nullptr;
  std::pmr::memory_resource* allocator_ = std::pmr::get_default_resource();
};

/// @brief Shared Redland-backed graph payload (model + PMR triple cache).
struct GraphData {
  RedlandStore store;
  std::pmr::vector<rdf::Triple> triples{std::pmr::get_default_resource()};

  explicit GraphData(RedlandStore store_in,
                     std::pmr::memory_resource* allocator)
      : store(std::move(store_in)),
        triples(std::pmr::polymorphic_allocator<rdf::Triple>{allocator}) {}
};

}  // namespace cccr::detail
