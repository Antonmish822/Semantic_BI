#pragma once

#include <cccr/bindings/c/status.h>
#include <cccr/ast/types.hpp>
#include <cccr/bindings/common/error.hpp>
#include <cccr/context.hpp>
#include <cccr/input/source.hpp>
#include <cccr/rdf/graph.hpp>
#include <cccr/rdf/syntax.hpp>
#include <cccr/sparql/results.hpp>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>

struct cccr_context {
  static constexpr size_t kDefaultPoolSize = 1024 * 1024;

  /// @brief Constructs a context; @p arena_bytes of 0 selects the default.
  /// @param arena_bytes Initial monotonic arena size in bytes
  explicit cccr_context(size_t arena_bytes = 0)
      : arena(arena_bytes == 0 ? kDefaultPoolSize : arena_bytes),
        core{.allocator = &arena} {}

  void SetError(cccr::bindings::BindingError error) {
    last_error.assign(cccr::bindings::BindingErrorToString(error));
  }

  void SetError(std::string_view message) { last_error.assign(message); }

  void ClearError() { last_error.clear(); }

  std::pmr::monotonic_buffer_resource arena;
  cccr::Context core;
  std::string last_error;
};

struct cccr_schema {
  cccr_context* ctx = nullptr;
  cccr::ast::Schema schema;
  std::filesystem::path include_base;
};

struct cccr_source {
  cccr_context* ctx = nullptr;
  std::unique_ptr<cccr::input::Source> source;
};

struct cccr_graph {
  cccr_context* ctx = nullptr;
  cccr::rdf::Graph graph;
};

[[nodiscard]] inline char* CccrDupString(std::string_view text) {
  auto* const out = static_cast<char*>(std::malloc(text.size() + 1));
  if (out == nullptr) [[unlikely]] {
    return nullptr;
  }
  std::memcpy(out, text.data(), text.size());
  out[text.size()] = '\0';
  return out;
}

[[nodiscard]] constexpr cccr::rdf::RdfSyntax CccrToRdfSyntax(
    int format) noexcept {
  switch (format) {
    case 1:
      return cccr::rdf::RdfSyntax::kTurtle;
    case 2:
      return cccr::rdf::RdfSyntax::kRdfXml;
    case 0:
    default:
      return cccr::rdf::RdfSyntax::kNTriples;
  }
}

[[nodiscard]] constexpr cccr::sparql::ResultFormat CccrToResultFormat(
    int format) noexcept {
  switch (format) {
    case 1:
      return cccr::sparql::ResultFormat::kXml;
    case 2:
      return cccr::sparql::ResultFormat::kHtml;
    case 0:
    default:
      return cccr::sparql::ResultFormat::kJson;
  }
}

[[nodiscard]] constexpr cccr_status CccrToStatus(
    cccr::bindings::BindingError error) noexcept {
  switch (error) {
    using enum cccr::bindings::BindingError;
    case kInvalidArg:
      return CCCR_ERR_INVALID_ARG;
    case kSchemaLoad:
      return CCCR_ERR_SCHEMA;
    case kInputLoad:
      return CCCR_ERR_INPUT;
    case kRdfLoad:
      return CCCR_ERR_RDF;
    case kEngineFailed:
      return CCCR_ERR_ENGINE;
    case kQueryFailed:
      return CCCR_ERR_QUERY;
    case kSerializeFailed:
      return CCCR_ERR_SERIALIZE;
    case kInternal:
    default:
      return CCCR_ERR_INTERNAL;
  }
}
