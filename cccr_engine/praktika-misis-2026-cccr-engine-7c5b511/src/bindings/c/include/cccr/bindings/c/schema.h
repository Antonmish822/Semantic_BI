#ifndef CCCR_BINDINGS_C_SCHEMA_H_
#define CCCR_BINDINGS_C_SCHEMA_H_

#include <cccr/bindings/c/context.h>
#include <cccr/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Opaque loaded CCCR schema.
typedef struct cccr_schema cccr_schema;

/// @brief Loads a CCCR schema from JSON text.
/// @param ctx Owning context (required)
/// @param json UTF-8 CCCR JSON schema text
/// @param include_base Optional include base directory; NULL skips
/// @return New schema handle, or NULL (see @ref cccr_context_last_error)
CCCR_C_API cccr_schema* cccr_schema_load(cccr_context* ctx, const char* json,
                                         const char* include_base);

/// @brief Destroys @p schema.
/// @param schema Schema handle; NULL is a no-op
CCCR_C_API void cccr_schema_destroy(cccr_schema* schema);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CCCR_BINDINGS_C_SCHEMA_H_ */
