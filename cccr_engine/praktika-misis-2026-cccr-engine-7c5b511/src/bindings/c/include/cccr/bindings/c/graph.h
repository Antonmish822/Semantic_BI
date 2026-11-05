#ifndef CCCR_BINDINGS_C_GRAPH_H_
#define CCCR_BINDINGS_C_GRAPH_H_

#include <cccr/bindings/c/context.h>
#include <cccr/bindings/c/export.h>
#include <cccr/bindings/c/schema.h>
#include <cccr/bindings/c/source.h>
#include <cccr/bindings/c/status.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Opaque RDF graph.
typedef struct cccr_graph cccr_graph;

/// @brief Loads an RDF document into a graph.
/// @param ctx Owning context (required)
/// @param text UTF-8 RDF document text
/// @param format Serialization of @p text
/// @return New graph handle, or NULL (see @ref cccr_context_last_error)
CCCR_C_API cccr_graph* cccr_graph_load(cccr_context* ctx, const char* text,
                                       cccr_graph_format format);

/// @brief Runs @p schema over @p source and returns the output graph.
/// @param ctx Owning context (required)
/// @param schema Loaded schema (required)
/// @param source Loaded input (required)
/// @param knowledge Optional seed graph; NULL skips
/// @return New graph handle, or NULL (see @ref cccr_context_last_error)
CCCR_C_API cccr_graph* cccr_engine_run(cccr_context* ctx,
                                       const cccr_schema* schema,
                                       cccr_source* source,
                                       const cccr_graph* knowledge);

/// @brief Runs @p schema over each source and merges the output graphs.
/// @param ctx Owning context (required)
/// @param schema Loaded schema (required)
/// @param sources Array of loaded inputs (required, non-empty)
/// @param source_count Number of entries in @p sources
/// @param knowledge Optional seed graph; NULL skips
/// @return New graph handle, or NULL (see @ref cccr_context_last_error)
CCCR_C_API cccr_graph* cccr_engine_run_many(cccr_context* ctx,
                                            const cccr_schema* schema,
                                            cccr_source* const* sources,
                                            size_t source_count,
                                            const cccr_graph* knowledge);

/// @brief Appends all triples from @p other into @p dest.
/// @param dest Destination graph (required, mutated)
/// @param other Source graph (required; must not alias @p dest)
/// @return @ref CCCR_OK on success; otherwise an error code (see also
///         @ref cccr_context_last_error on @p dest's context)
CCCR_C_API cccr_status cccr_graph_merge(cccr_graph* dest,
                                        const cccr_graph* other);

/// @brief Serializes @p graph to RDF/XML.
/// @param ctx Context used for error reporting (required)
/// @param graph Graph to serialize (required)
/// @return Heap-owned RDF/XML (free with @ref cccr_string_free), or NULL
CCCR_C_API char* cccr_graph_to_rdfxml(cccr_context* ctx,
                                      const cccr_graph* graph);

/// @brief Runs a SPARQL SELECT over @p graph.
/// @param ctx Context used for error reporting (required)
/// @param graph Graph to query (required)
/// @param sparql SPARQL query text (required)
/// @param format Result format
/// @return Heap-owned result text (free with @ref cccr_string_free), or NULL
CCCR_C_API char* cccr_graph_query(cccr_context* ctx, const cccr_graph* graph,
                                  const char* sparql,
                                  cccr_result_format format);

/// @brief Destroys @p graph.
/// @param graph Graph handle; NULL is a no-op
CCCR_C_API void cccr_graph_destroy(cccr_graph* graph);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CCCR_BINDINGS_C_GRAPH_H_ */
