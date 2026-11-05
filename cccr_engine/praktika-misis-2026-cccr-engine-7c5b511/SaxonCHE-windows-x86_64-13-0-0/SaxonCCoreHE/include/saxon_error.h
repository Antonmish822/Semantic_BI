#ifndef SAXON_ERROR_H
#define SAXON_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SAXON_OK = 0,
  SAXON_ERR_STATIC = -3,
  SAXON_ERR_RUNTIME = -2,
  SAXON_ERR_INTERNAL = -4
} saxon_status_t;

typedef struct saxon_error_t {
  saxon_status_t status;
  const char *error_code;
  const char *message;
  const char *system_id;
  int line_number;
  int num_of_errors;
} saxon_error_t;

// SAXONC_CORE_EXPORT extern saxon_error_t SAXON_ERROR_DEFAULT;

#ifdef __cplusplus
}
#endif

#endif
