#include <cccr/bindings/c/graph.h>

#include "handles.hpp"

#include <cccr/bindings/common/pipeline.hpp>

#include <functional>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] cccr_graph* CccrMakeGraph(cccr_context* ctx,
                                        cccr::rdf::Graph graph) {
  try {
    auto* const out = new cccr_graph{};
    out->ctx = ctx;
    out->graph = std::move(graph);
    return out;
  } catch (...) {
    ctx->SetError(cccr::bindings::BindingError::kInternal);
    return nullptr;
  }
}

}  // namespace

extern "C" {

cccr_graph* cccr_graph_load(cccr_context* ctx, const char* text,
                            cccr_graph_format format) {
  if (ctx == nullptr || text == nullptr) [[unlikely]] {
    if (ctx != nullptr) {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return nullptr;
  }
  ctx->ClearError();

  auto loaded =
      cccr::bindings::LoadGraph(ctx->core, text, CccrToRdfSyntax(format));
  if (!loaded) [[unlikely]] {
    ctx->SetError(loaded.error());
    return nullptr;
  }

  return CccrMakeGraph(ctx, std::move(*loaded));
}

cccr_graph* cccr_engine_run(cccr_context* ctx, const cccr_schema* schema,
                            cccr_source* source, const cccr_graph* knowledge) {
  if (source == nullptr) [[unlikely]] {
    if (ctx != nullptr) {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return nullptr;
  }
  return cccr_engine_run_many(ctx, schema, &source, 1, knowledge);
}

cccr_graph* cccr_engine_run_many(cccr_context* ctx, const cccr_schema* schema,
                                 cccr_source* const* sources,
                                 size_t source_count,
                                 const cccr_graph* knowledge) {
  if (ctx == nullptr || schema == nullptr || sources == nullptr ||
      source_count == 0) [[unlikely]] {
    if (ctx != nullptr) {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return nullptr;
  }

  if (schema->ctx != ctx || (knowledge != nullptr && knowledge->ctx != ctx))
      [[unlikely]] {
    ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    return nullptr;
  }

  std::vector<std::reference_wrapper<cccr::input::Source>> refs;
  refs.reserve(source_count);
  for (size_t i = 0; i < source_count; ++i) {
    cccr_source* const source = sources[i];
    if (source == nullptr || source->source == nullptr || source->ctx != ctx)
        [[unlikely]] {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
      return nullptr;
    }
    refs.emplace_back(*source->source);
  }
  ctx->ClearError();

  const cccr::rdf::Graph* knowledge_ptr =
      knowledge != nullptr ? &knowledge->graph : nullptr;

  auto ran = cccr::bindings::RunEngine(ctx->core, schema->schema, refs,
                                       knowledge_ptr, schema->include_base);
  if (!ran) [[unlikely]] {
    ctx->SetError(ran.error());
    return nullptr;
  }

  return CccrMakeGraph(ctx, std::move(*ran));
}

cccr_status cccr_graph_merge(cccr_graph* dest, const cccr_graph* other) {
  if (dest == nullptr || other == nullptr || dest->ctx == nullptr)
      [[unlikely]] {
    if (dest != nullptr && dest->ctx != nullptr) {
      dest->ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return CCCR_ERR_INVALID_ARG;
  }

  if (other->ctx != dest->ctx) [[unlikely]] {
    dest->ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    return CCCR_ERR_INVALID_ARG;
  }
  dest->ctx->ClearError();

  auto merged = cccr::bindings::MergeGraph(dest->graph, other->graph);
  if (!merged) [[unlikely]] {
    dest->ctx->SetError(merged.error());
    return CccrToStatus(merged.error());
  }
  return CCCR_OK;
}

char* cccr_graph_to_rdfxml(cccr_context* ctx, const cccr_graph* graph) {
  if (ctx == nullptr || graph == nullptr) [[unlikely]] {
    if (ctx != nullptr) {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return nullptr;
  }
  ctx->ClearError();

  auto serialized = cccr::bindings::SerializeRdfXml(graph->graph);
  if (!serialized) [[unlikely]] {
    ctx->SetError(serialized.error());
    return nullptr;
  }

  char* const out = CccrDupString(*serialized);
  if (out == nullptr) [[unlikely]] {
    ctx->SetError(cccr::bindings::BindingError::kInternal);
  }
  return out;
}

char* cccr_graph_query(cccr_context* ctx, const cccr_graph* graph,
                       const char* sparql, cccr_result_format format) {
  if (ctx == nullptr || graph == nullptr || sparql == nullptr) [[unlikely]] {
    if (ctx != nullptr) {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return nullptr;
  }
  ctx->ClearError();

  auto result = cccr::bindings::QueryGraph(ctx->core, graph->graph, sparql,
                                           CccrToResultFormat(format));
  if (!result) [[unlikely]] {
    ctx->SetError(result.error());
    return nullptr;
  }

  char* const out = CccrDupString(*result);
  if (out == nullptr) [[unlikely]] {
    ctx->SetError(cccr::bindings::BindingError::kInternal);
  }
  return out;
}

void cccr_graph_destroy(cccr_graph* graph) {
  delete graph;
}

}  // extern "C"
