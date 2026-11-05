#ifndef CCCR_BINDINGS_C_CONTEXT_H_
#define CCCR_BINDINGS_C_CONTEXT_H_

#include <cccr/bindings/c/export.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Default initial arena size used by @ref cccr_context_create (1 MiB).
#define CCCR_CONTEXT_DEFAULT_ARENA_BYTES ((size_t)(1024 * 1024))

/// @brief Opaque runtime context (allocator arena + last error).
typedef struct cccr_context cccr_context;

/// @brief Creates a new context with the default arena size.
/// @return New context, or NULL on allocation failure
CCCR_C_API cccr_context* cccr_context_create(void);

/// @brief Creates a new context with a custom initial arena size.
/// @param arena_bytes Initial monotonic arena size; 0 selects the default
/// @return New context, or NULL on allocation failure
CCCR_C_API cccr_context* cccr_context_create_with_arena(size_t arena_bytes);

/// @brief Destroys @p ctx and its arena.
/// @warning All schema/source/graph handles from @p ctx must already be
///          destroyed.
/// @param ctx Context to destroy; NULL is a no-op
CCCR_C_API void cccr_context_destroy(cccr_context* ctx);

/// @brief Returns the last error message recorded on @p ctx.
/// @param ctx Context
/// @return NUL-terminated message (empty when none); owned by @p ctx
CCCR_C_API const char* cccr_context_last_error(const cccr_context* ctx);

/// @brief Clears the last error message on @p ctx.
/// @param ctx Context; NULL is a no-op
CCCR_C_API void cccr_context_clear_error(cccr_context* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CCCR_BINDINGS_C_CONTEXT_H_ */
