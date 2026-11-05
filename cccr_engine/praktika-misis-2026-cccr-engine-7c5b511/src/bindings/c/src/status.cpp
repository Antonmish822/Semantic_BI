#include <cccr/bindings/c/status.h>

#include <cstdlib>

#ifndef CCCR_PROJECT_VERSION
#define CCCR_PROJECT_VERSION "0.0.0"
#endif

extern "C" {

const char* cccr_version(void) {
  return CCCR_PROJECT_VERSION;
}

const char* cccr_status_string(cccr_status status) {
  switch (status) {
    case CCCR_OK:
      return "OK";
    case CCCR_ERR_INVALID_ARG:
      return "Invalid argument";
    case CCCR_ERR_SCHEMA:
      return "Failed to load schema";
    case CCCR_ERR_INPUT:
      return "Failed to load input";
    case CCCR_ERR_RDF:
      return "Failed to load RDF data";
    case CCCR_ERR_ENGINE:
      return "Engine run failed";
    case CCCR_ERR_QUERY:
      return "SPARQL query failed";
    case CCCR_ERR_SERIALIZE:
      return "RDF/XML serialization failed";
    case CCCR_ERR_INTERNAL:
    default:
      return "Internal bindings error";
  }
}

void cccr_string_free(char* str) {
  std::free(str);
}

}  // extern "C"
