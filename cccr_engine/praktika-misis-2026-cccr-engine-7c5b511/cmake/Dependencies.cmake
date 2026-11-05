# Vendored third-party libraries under third-party/

set(CCCR_THIRD_PARTY_DIR "${PROJECT_SOURCE_DIR}/third-party")

# Persistent download/build cache (survives deleting build/). Override with -DCCCR_CACHE_DIR=...
set(CCCR_CACHE_DIR "${PROJECT_SOURCE_DIR}/.cache" CACHE PATH
  "Persistent cache for fetched/built third-party deps")
file(MAKE_DIRECTORY "${CCCR_CACHE_DIR}")

# glaze — always required by core (header-only).
set(glaze_INSTALL OFF CACHE BOOL "Install glaze" FORCE)
add_subdirectory(
    "${CCCR_THIRD_PARTY_DIR}/glaze"
    "${CMAKE_BINARY_DIR}/_deps/glaze"
    EXCLUDE_FROM_ALL
)

# SaxonC-HE is mandatory (XQuery). Upstream ships shared libs only.
include(FindOrFetchSaxonCHE)
cccr_find_or_fetch_saxon_che()

# Redland stack (raptor2 + rasqal + librdf) — always required by core.
# System packages → shared; vendored fallback → static (see BuildRedland.cmake).
include(BuildRedland)

if(CCCR_BUILD_CLI)
  set(ARGPARSE_INSTALL OFF CACHE BOOL "Install argparse" FORCE)
  set(ARGPARSE_BUILD_TESTS OFF CACHE BOOL "Build argparse tests" FORCE)
  set(ARGPARSE_BUILD_SAMPLES OFF CACHE BOOL "Build argparse samples" FORCE)
  add_subdirectory(
      "${CCCR_THIRD_PARTY_DIR}/argparse"
      "${CMAKE_BINARY_DIR}/_deps/argparse"
      EXCLUDE_FROM_ALL
  )
endif()

if(CCCR_BUILD_BINDINGS_PYTHON)
  set(PYBIND11_INSTALL OFF CACHE BOOL "Install pybind11" FORCE)
  set(PYBIND11_TEST OFF CACHE BOOL "Build pybind11 tests" FORCE)
  add_subdirectory(
      "${CCCR_THIRD_PARTY_DIR}/pybind11"
      "${CMAKE_BINARY_DIR}/_deps/pybind11"
      EXCLUDE_FROM_ALL
  )
endif()

if(CCCR_BUILD_TESTS)
  set(DOCTEST_WITH_TESTS OFF CACHE BOOL "Build doctest's own tests" FORCE)
  set(DOCTEST_NO_INSTALL ON CACHE BOOL "Skip doctest install" FORCE)
  add_subdirectory(
      "${CCCR_THIRD_PARTY_DIR}/doctest"
      "${CMAKE_BINARY_DIR}/_deps/doctest"
      EXCLUDE_FROM_ALL
  )
endif()
