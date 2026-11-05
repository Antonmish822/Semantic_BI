#ifndef CCCR_BINDINGS_C_SOURCE_H_
#define CCCR_BINDINGS_C_SOURCE_H_

#include <cccr/bindings/c/context.h>
#include <cccr/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Opaque loaded input document (XML or CSV).
typedef struct cccr_source cccr_source;

/// @brief Loads an XML/HTML document.
/// @param ctx Owning context (required)
/// @param xml UTF-8 XML text
/// @return New source handle, or NULL (see @ref cccr_context_last_error)
CCCR_C_API cccr_source* cccr_source_load_xml(cccr_context* ctx,
                                             const char* xml);

/// @brief Loads a CSV document.
/// @param ctx Owning context (required)
/// @param csv UTF-8 CSV text
/// @return New source handle, or NULL (see @ref cccr_context_last_error)
CCCR_C_API cccr_source* cccr_source_load_csv(cccr_context* ctx,
                                             const char* csv);

/// @brief Destroys @p source.
/// @param source Source handle; NULL is a no-op
CCCR_C_API void cccr_source_destroy(cccr_source* source);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CCCR_BINDINGS_C_SOURCE_H_ */
