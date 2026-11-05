#include <cccr/bindings/c/source.h>

#include "handles.hpp"

#include <cccr/bindings/common/pipeline.hpp>

#include <new>
#include <utility>

extern "C" {

cccr_source* cccr_source_load_xml(cccr_context* ctx, const char* xml) {
  if (ctx == nullptr || xml == nullptr) [[unlikely]] {
    if (ctx != nullptr) {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return nullptr;
  }
  ctx->ClearError();

  auto loaded = cccr::bindings::LoadXmlSource(ctx->core, xml);
  if (!loaded) [[unlikely]] {
    ctx->SetError(loaded.error());
    return nullptr;
  }

  try {
    auto* const out = new cccr_source{};
    out->ctx = ctx;
    out->source = std::move(*loaded);
    return out;
  } catch (...) {
    ctx->SetError(cccr::bindings::BindingError::kInternal);
    return nullptr;
  }
}

cccr_source* cccr_source_load_csv(cccr_context* ctx, const char* csv) {
  if (ctx == nullptr || csv == nullptr) [[unlikely]] {
    if (ctx != nullptr) {
      ctx->SetError(cccr::bindings::BindingError::kInvalidArg);
    }
    return nullptr;
  }
  ctx->ClearError();

  auto loaded = cccr::bindings::LoadCsvSource(ctx->core, csv);
  if (!loaded) [[unlikely]] {
    ctx->SetError(loaded.error());
    return nullptr;
  }

  try {
    auto* const out = new cccr_source{};
    out->ctx = ctx;
    out->source = std::move(*loaded);
    return out;
  } catch (...) {
    ctx->SetError(cccr::bindings::BindingError::kInternal);
    return nullptr;
  }
}

void cccr_source_destroy(cccr_source* source) {
  delete source;
}

}  // extern "C"
