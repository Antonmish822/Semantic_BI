#ifndef CCCR_BINDINGS_C_STATUS_H_
#define CCCR_BINDINGS_C_STATUS_H_

#include <cccr/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Status codes returned by the CCCR C API.
typedef enum cccr_status {
  CCCR_OK = 0,
  CCCR_ERR_INVALID_ARG = 1,
  CCCR_ERR_SCHEMA = 2,
  CCCR_ERR_INPUT = 3,
  CCCR_ERR_RDF = 4,
  CCCR_ERR_ENGINE = 5,
  CCCR_ERR_QUERY = 6,
  CCCR_ERR_SERIALIZE = 7,
  CCCR_ERR_INTERNAL = 8,
} cccr_status;

/// @brief RDF serialization for graph load.
typedef enum cccr_graph_format {
  CCCR_GRAPH_NTRIPLES = 0,
  CCCR_GRAPH_TURTLE = 1,
  CCCR_GRAPH_RDFXML = 2,
} cccr_graph_format;

/// @brief SPARQL SELECT result format.
typedef enum cccr_result_format {
  CCCR_RESULT_JSON = 0,
  CCCR_RESULT_XML = 1,
  CCCR_RESULT_HTML = 2,
} cccr_result_format;

/// @brief Returns the library version string (e.g. `"0.1.0"`).
/// @return NUL-terminated version; owned by the library
CCCR_C_API const char* cccr_version(void);

/// @brief Converts @p status to a short diagnostic string.
/// @param status Status code
/// @return NUL-terminated message; owned by the library
CCCR_C_API const char* cccr_status_string(cccr_status status);

/// @brief Frees a string returned by graph serialize/query helpers.
/// @param str String previously returned, or NULL
CCCR_C_API void cccr_string_free(char* str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CCCR_BINDINGS_C_STATUS_H_ */
