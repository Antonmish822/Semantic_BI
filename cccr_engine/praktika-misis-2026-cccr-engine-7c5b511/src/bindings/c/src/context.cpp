#include <cccr/bindings/c/context.h>

#include "handles.hpp"

#include <cstddef>

extern "C" {

cccr_context* cccr_context_create(void) {
  return cccr_context_create_with_arena(0);
}

cccr_context* cccr_context_create_with_arena(size_t arena_bytes) {
  try {
    return new cccr_context{arena_bytes};
  } catch (...) {
    return nullptr;
  }
}

void cccr_context_destroy(cccr_context* ctx) {
  delete ctx;
}

const char* cccr_context_last_error(const cccr_context* ctx) {
  if (ctx == nullptr) [[unlikely]] {
    return "";
  }
  return ctx->last_error.c_str();
}

void cccr_context_clear_error(cccr_context* ctx) {
  if (ctx == nullptr) [[unlikely]] {
    return;
  }
  ctx->ClearError();
}

}  // extern "C"
