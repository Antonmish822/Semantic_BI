#ifndef CCCR_BINDINGS_C_EXPORT_H_
#define CCCR_BINDINGS_C_EXPORT_H_

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef CCCR_BINDINGS_C_BUILDING
#define CCCR_C_API __declspec(dllexport)
#else
#define CCCR_C_API __declspec(dllimport)
#endif
#else
#define CCCR_C_API __attribute__((visibility("default")))
#endif

#endif /* CCCR_BINDINGS_C_EXPORT_H_ */
