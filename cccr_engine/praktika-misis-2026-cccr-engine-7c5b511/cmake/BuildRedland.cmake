# Resolve Redland stack (raptor2 → rasqal → librdf).
# Produces imported targets:
#   Redland::raptor2  Redland::rasqal  Redland::redland
#   cccr::redland (umbrella)
#
# Resolution order (Linux-primary):
#   1. System packages via pkg-config (unless CCCR_REDLAND_USE_SYSTEM=OFF)
#      → always shared (.so) linkage
#   2. Vendored ExternalProject build (third-party/ → build-tree prefix)
#      → always static (.a) linkage; system transitive deps stay shared
#
# Sets:
#   CCCR_REDLAND_VENDORED       — TRUE when building from source
#   CCCR_REDLAND_LINKED_STATIC  — TRUE when umbrella uses .a archives
#   CCCR_REDLAND_PREFIX         — vendored install prefix (else empty)
#
# Windows is unsupported. SaxonC-HE is unrelated (always shared).

option(CCCR_REDLAND_USE_SYSTEM
  "Prefer system Redland (raptor2/rasqal/redland) via pkg-config" ON)

include(FindPkgConfig)

if(WIN32)
  message(FATAL_ERROR
    "Redland (raptor2/rasqal/librdf) is not supported on Windows. "
    "Build CCCR on Linux (or WSL).")
endif()

set(CCCR_REDLAND_VENDORED FALSE)
set(CCCR_REDLAND_LINKED_STATIC FALSE)
set(CCCR_REDLAND_PREFIX "")

# ---------------------------------------------------------------------------
# Helpers (vendored-static transitive deps)
# ---------------------------------------------------------------------------

function(cccr_redland_find_shared out_var)
  set(_names ${ARGN})
  string(JOIN "_" _cache_key ${_names})
  string(MAKE_C_IDENTIFIER "CCCR_REDLAND_DEP_${_cache_key}" _cache_var)

  if(NOT ${_cache_var})
    set(_saved_suffixes "${CMAKE_FIND_LIBRARY_SUFFIXES}")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".so.2" ".so.1" ".so.0" ".a")
    find_library(${_cache_var} NAMES ${_names})
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${_saved_suffixes}")

    if(NOT ${_cache_var})
      set(_found "")
      foreach(_name IN LISTS _names)
        foreach(_dir IN ITEMS
            /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
            /usr/lib64
            /usr/lib)
          foreach(_cand IN ITEMS
              "${_dir}/lib${_name}.so"
              "${_dir}/lib${_name}.so.2"
              "${_dir}/lib${_name}.so.1"
              "${_dir}/lib${_name}.so.0")
            if(EXISTS "${_cand}")
              set(_found "${_cand}")
              break()
            endif()
          endforeach()
          if(_found)
            break()
          endif()
        endforeach()
        if(_found)
          break()
        endif()
      endforeach()
      if(_found)
        set(${_cache_var} "${_found}" CACHE FILEPATH
          "Redland transitive dependency (${_names})" FORCE)
      endif()
    endif()
  endif()

  set(${out_var} "${${_cache_var}}" PARENT_SCOPE)
endfunction()

function(cccr_redland_import_static_archives raptor_a rasqal_a rdf_a)
  cmake_parse_arguments(ARG "" "" "INCLUDES;TRANSITIVE" ${ARGN})

  add_library(Redland::raptor2 STATIC IMPORTED GLOBAL)
  set_target_properties(Redland::raptor2 PROPERTIES
    IMPORTED_LOCATION "${raptor_a}"
  )
  target_include_directories(Redland::raptor2 INTERFACE ${ARG_INCLUDES})

  add_library(Redland::rasqal STATIC IMPORTED GLOBAL)
  set_target_properties(Redland::rasqal PROPERTIES
    IMPORTED_LOCATION "${rasqal_a}"
  )
  target_include_directories(Redland::rasqal INTERFACE ${ARG_INCLUDES})
  target_link_libraries(Redland::rasqal INTERFACE Redland::raptor2)

  add_library(Redland::redland STATIC IMPORTED GLOBAL)
  set_target_properties(Redland::redland PROPERTIES
    IMPORTED_LOCATION "${rdf_a}"
  )
  target_include_directories(Redland::redland INTERFACE ${ARG_INCLUDES})
  target_link_libraries(Redland::redland INTERFACE Redland::rasqal)

  add_library(cccr_redland INTERFACE)
  add_library(cccr::redland ALIAS cccr_redland)
  target_link_libraries(cccr_redland INTERFACE
      Redland::redland
      ${ARG_TRANSITIVE}
  )
  target_include_directories(cccr_redland INTERFACE ${ARG_INCLUDES})

  set(CCCR_REDLAND_LINKED_STATIC TRUE PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# System packages → shared
# ---------------------------------------------------------------------------

set(_cccr_redland_system_ok FALSE)
if(CCCR_REDLAND_USE_SYSTEM)
  pkg_check_modules(CCCR_SYSTEM_RAPTOR2 QUIET IMPORTED_TARGET raptor2)
  pkg_check_modules(CCCR_SYSTEM_RASQAL QUIET IMPORTED_TARGET rasqal)
  pkg_check_modules(CCCR_SYSTEM_REDLAND QUIET IMPORTED_TARGET redland)

  if(CCCR_SYSTEM_RAPTOR2_FOUND AND CCCR_SYSTEM_RASQAL_FOUND
      AND CCCR_SYSTEM_REDLAND_FOUND)
    set(_cccr_redland_system_ok TRUE)
  else()
    set(_cccr_missing "")
    if(NOT CCCR_SYSTEM_RAPTOR2_FOUND)
      list(APPEND _cccr_missing "raptor2")
    endif()
    if(NOT CCCR_SYSTEM_RASQAL_FOUND)
      list(APPEND _cccr_missing "rasqal")
    endif()
    if(NOT CCCR_SYSTEM_REDLAND_FOUND)
      list(APPEND _cccr_missing "redland")
    endif()
    list(JOIN _cccr_missing ", " _cccr_missing_str)
    message(STATUS
      "Redland: system pkg-config incomplete (missing: ${_cccr_missing_str}); "
      "falling back to vendored static build. "
      "Install libraptor2-dev librasqal3-dev librdf0-dev (Debian/Ubuntu) "
      "or set CCCR_REDLAND_USE_SYSTEM=OFF to skip the probe.")
  endif()
endif()

if(_cccr_redland_system_ok)
  add_library(Redland::raptor2 INTERFACE IMPORTED GLOBAL)
  set_target_properties(Redland::raptor2 PROPERTIES
    INTERFACE_LINK_LIBRARIES PkgConfig::CCCR_SYSTEM_RAPTOR2)

  add_library(Redland::rasqal INTERFACE IMPORTED GLOBAL)
  set_target_properties(Redland::rasqal PROPERTIES
    INTERFACE_LINK_LIBRARIES "PkgConfig::CCCR_SYSTEM_RASQAL;Redland::raptor2")

  add_library(Redland::redland INTERFACE IMPORTED GLOBAL)
  set_target_properties(Redland::redland PROPERTIES
    INTERFACE_LINK_LIBRARIES
      "PkgConfig::CCCR_SYSTEM_REDLAND;Redland::rasqal;Redland::raptor2")

  add_library(cccr_redland INTERFACE)
  add_library(cccr::redland ALIAS cccr_redland)
  target_link_libraries(cccr_redland INTERFACE Redland::redland)

  message(STATUS
    "Redland: system shared packages "
    "(raptor2 ${CCCR_SYSTEM_RAPTOR2_VERSION}, "
    "rasqal ${CCCR_SYSTEM_RASQAL_VERSION}, "
    "redland ${CCCR_SYSTEM_REDLAND_VERSION})")
else()
  # -------------------------------------------------------------------------
  # Vendored ExternalProject → static archives
  # -------------------------------------------------------------------------
  find_program(CCCR_MAKE_PROGRAM NAMES gmake make REQUIRED)
  pkg_check_modules(CCCR_LIBXML2 REQUIRED libxml-2.0)
  pkg_check_modules(CCCR_LIBPCRE libpcre)
  if(NOT CCCR_LIBPCRE_FOUND)
    message(WARNING
      "libpcre (PCRE1) not found via pkg-config; rasqal configure may fail. "
      "Install libpcre3-dev (Debian/Ubuntu) or equivalent.")
  endif()

  include(ExternalProject)

  set(CCCR_REDLAND_VENDORED TRUE)
  set(CCCR_REDLAND_PREFIX "${CMAKE_BINARY_DIR}/_deps/redland-prefix")
  set(CCCR_REDLAND_BUILD "${CMAKE_BINARY_DIR}/_deps/redland-build")
  set(CCCR_RAPTOR2_SOURCE_DIR "${CCCR_THIRD_PARTY_DIR}/raptor2")
  set(CCCR_RASQAL_SOURCE_DIR "${CCCR_THIRD_PARTY_DIR}/rasqal")
  set(CCCR_REDLAND_SOURCE_DIR "${CCCR_THIRD_PARTY_DIR}/redland")

  set(_cccr_rl_raptor_lib "${CCCR_REDLAND_PREFIX}/lib/libraptor2.a")
  set(_cccr_rl_rasqal_lib "${CCCR_REDLAND_PREFIX}/lib/librasqal.a")
  set(_cccr_rl_rdf_lib "${CCCR_REDLAND_PREFIX}/lib/librdf.a")

  foreach(_dir IN ITEMS
      "${CCCR_REDLAND_PREFIX}/include"
      "${CCCR_REDLAND_PREFIX}/include/raptor2"
      "${CCCR_REDLAND_PREFIX}/include/rasqal"
      "${CCCR_REDLAND_PREFIX}/lib"
      "${CCCR_REDLAND_BUILD}/raptor2"
      "${CCCR_REDLAND_BUILD}/rasqal"
      "${CCCR_REDLAND_BUILD}/redland")
    file(MAKE_DIRECTORY "${_dir}")
  endforeach()

  set(_cccr_pkg_config_path
      "${CCCR_REDLAND_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

  ExternalProject_Add(cccr_raptor2_ep
    SOURCE_DIR "${CCCR_RAPTOR2_SOURCE_DIR}"
    BINARY_DIR "${CCCR_REDLAND_BUILD}/raptor2"
    CONFIGURE_COMMAND
      "${CCCR_RAPTOR2_SOURCE_DIR}/configure"
        --prefix=${CCCR_REDLAND_PREFIX}
        --enable-static
        --disable-shared
        --disable-gtk-doc
    BUILD_COMMAND "${CCCR_MAKE_PROGRAM}" -j
    INSTALL_COMMAND "${CCCR_MAKE_PROGRAM}" install
    BUILD_BYPRODUCTS
      "${_cccr_rl_raptor_lib}"
      "${CCCR_REDLAND_PREFIX}/lib/pkgconfig/raptor2.pc"
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
  )

  ExternalProject_Add(cccr_rasqal_ep
    SOURCE_DIR "${CCCR_RASQAL_SOURCE_DIR}"
    BINARY_DIR "${CCCR_REDLAND_BUILD}/rasqal"
    DEPENDS cccr_raptor2_ep
    CONFIGURE_COMMAND
      ${CMAKE_COMMAND} -E env
        "PKG_CONFIG_PATH=${_cccr_pkg_config_path}"
        "LD_LIBRARY_PATH=${CCCR_REDLAND_PREFIX}/lib:$ENV{LD_LIBRARY_PATH}"
      "${CCCR_RASQAL_SOURCE_DIR}/configure"
        --prefix=${CCCR_REDLAND_PREFIX}
        --enable-static
        --disable-shared
        --disable-gtk-doc
        --with-raptor=system
    BUILD_COMMAND
      ${CMAKE_COMMAND} -E env
        "PKG_CONFIG_PATH=${_cccr_pkg_config_path}"
        "LD_LIBRARY_PATH=${CCCR_REDLAND_PREFIX}/lib:$ENV{LD_LIBRARY_PATH}"
      "${CCCR_MAKE_PROGRAM}" -j
    INSTALL_COMMAND
      ${CMAKE_COMMAND} -E env
        "PKG_CONFIG_PATH=${_cccr_pkg_config_path}"
        "LD_LIBRARY_PATH=${CCCR_REDLAND_PREFIX}/lib:$ENV{LD_LIBRARY_PATH}"
      "${CCCR_MAKE_PROGRAM}" install
    BUILD_BYPRODUCTS
      "${_cccr_rl_rasqal_lib}"
      "${CCCR_REDLAND_PREFIX}/lib/pkgconfig/rasqal.pc"
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
  )

  ExternalProject_Add(cccr_redland_ep
    SOURCE_DIR "${CCCR_REDLAND_SOURCE_DIR}"
    BINARY_DIR "${CCCR_REDLAND_BUILD}/redland"
    DEPENDS cccr_rasqal_ep
    CONFIGURE_COMMAND
      ${CMAKE_COMMAND} -E env
        "PKG_CONFIG_PATH=${_cccr_pkg_config_path}"
        "LD_LIBRARY_PATH=${CCCR_REDLAND_PREFIX}/lib:$ENV{LD_LIBRARY_PATH}"
      "${CCCR_REDLAND_SOURCE_DIR}/configure"
        --prefix=${CCCR_REDLAND_PREFIX}
        --enable-static
        --disable-shared
        --disable-gtk-doc
        --with-raptor=system
        --with-rasqal=system
        --with-sqlite=no
        --with-mysql=no
        --with-postgresql=no
        --with-bdb=no
        --with-virtuoso=no
    BUILD_COMMAND
      ${CMAKE_COMMAND} -E env
        "PKG_CONFIG_PATH=${_cccr_pkg_config_path}"
        "LD_LIBRARY_PATH=${CCCR_REDLAND_PREFIX}/lib:$ENV{LD_LIBRARY_PATH}"
      "${CCCR_MAKE_PROGRAM}" -j
    INSTALL_COMMAND
      ${CMAKE_COMMAND} -E env
        "PKG_CONFIG_PATH=${_cccr_pkg_config_path}"
        "LD_LIBRARY_PATH=${CCCR_REDLAND_PREFIX}/lib:$ENV{LD_LIBRARY_PATH}"
      "${CCCR_MAKE_PROGRAM}" install
    BUILD_BYPRODUCTS
      "${_cccr_rl_rdf_lib}"
      "${CCCR_REDLAND_PREFIX}/lib/pkgconfig/redland.pc"
      "${CCCR_REDLAND_PREFIX}/include/redland.h"
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON
  )

  set(_cccr_vendored_includes
    "${CCCR_REDLAND_PREFIX}/include"
    "${CCCR_REDLAND_PREFIX}/include/raptor2"
    "${CCCR_REDLAND_PREFIX}/include/rasqal"
  )

  cccr_redland_find_shared(_curl curl)
  cccr_redland_find_shared(_xml2 xml2)
  cccr_redland_find_shared(_xslt xslt)
  cccr_redland_find_shared(_yajl yajl)
  cccr_redland_find_shared(_icuuc icuuc)
  cccr_redland_find_shared(_icudata icudata)
  cccr_redland_find_shared(_gmp gmp)
  cccr_redland_find_shared(_mhash mhash)
  cccr_redland_find_shared(_ltdl ltdl)
  set(_cccr_transitive "")
  foreach(_lib IN ITEMS
      "${_curl}" "${_xml2}" "${_xslt}" "${_yajl}" "${_icuuc}" "${_icudata}"
      "${_gmp}" "${_mhash}" "${_ltdl}" m)
    if(_lib)
      list(APPEND _cccr_transitive "${_lib}")
    endif()
  endforeach()

  cccr_redland_import_static_archives(
    "${_cccr_rl_raptor_lib}" "${_cccr_rl_rasqal_lib}" "${_cccr_rl_rdf_lib}"
    INCLUDES ${_cccr_vendored_includes}
    TRANSITIVE ${_cccr_transitive})
  add_dependencies(Redland::raptor2 cccr_raptor2_ep)
  add_dependencies(Redland::rasqal cccr_rasqal_ep)
  add_dependencies(Redland::redland cccr_redland_ep)

  message(STATUS
    "Redland: building vendored static raptor2/rasqal/librdf into "
    "${CCCR_REDLAND_PREFIX}")
endif()

set(CCCR_REDLAND_VENDORED "${CCCR_REDLAND_VENDORED}" CACHE INTERNAL
  "TRUE when Redland is built from third-party/")
set(CCCR_REDLAND_LINKED_STATIC "${CCCR_REDLAND_LINKED_STATIC}" CACHE INTERNAL
  "TRUE when cccr::redland links static raptor2/rasqal/librdf archives")
set(CCCR_REDLAND_PREFIX "${CCCR_REDLAND_PREFIX}" CACHE INTERNAL
  "Vendored Redland install prefix (empty when using system packages)")

unset(_cccr_redland_system_ok)
