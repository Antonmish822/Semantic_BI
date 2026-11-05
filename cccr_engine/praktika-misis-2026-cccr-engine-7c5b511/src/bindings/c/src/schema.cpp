#include <cccr/bindings/c/schema.h>

#include "handles.hpp"

#include <cccr/bindings/common/pipeline.hpp>

#include <filesystem>
#include <new>
#include <utility>

extern "C" {

cccr_schema* cccr_schema_load(cccr_context* ctx, const char* json,
                              const char* include_base) {
  if (ctx == nullptr || json == nullptr) [[unlikely]] {
    if (ctx != nullptr) {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return nullptr;
  }
  ctx->ClearError();

  const auto base = include_base != nullptr
                        ? std::filesystem::path{include_base}
                        : std::filesystem::path{};

  auto loaded = cccr::bindings::LoadSchema(ctx->core, json, base);
  if (!loaded) [[unlikely]] {
    ctx->SetError(loaded.error());
    return nullptr;
  }

  try {
    auto* const out = new cccr_schema{};
    out->ctx = ctx;
    out->schema = std::move(*loaded);
    out->include_base = base;
    return out;
  } catch (...) {
    ctx->SetError(cccr::bindings::BindingError::kInternal);
    return nullptr;
  }
}

void cccr_schema_destroy(cccr_schema* schema) {
  delete schema;
}

}  // extern "C"
