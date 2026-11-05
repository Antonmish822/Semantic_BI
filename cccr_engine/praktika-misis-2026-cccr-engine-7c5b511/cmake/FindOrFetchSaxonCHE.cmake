# Find or download SaxonC-HE 13 (mandatory for cccr_core).
#
# Resolution order:
#   1. Existing find_package(SaxonCHE) on CMAKE_PREFIX_PATH / CCCR_SAXON_CHE_ROOT / SAXONC_HOME
#   2. Download host-platform zip into ${CCCR_CACHE_DIR}/saxonc-he (survives build/ wipes)
#
# After success, SaxonCHE::saxonc-he is available and CCCR_SAXON_CHE_ROOT is set.

set(CCCR_SAXON_CHE_VERSION "13.0.0" CACHE STRING "SaxonC-HE version to require")
set(CCCR_SAXON_CHE_ROOT "" CACHE PATH
    "Pre-extracted SaxonC-HE root (directory containing SaxonCHE/ and SaxonCCoreHE/)")
option(CCCR_SAXON_OFFLINE "Disable auto-download of SaxonC-HE; require CCCR_SAXON_CHE_ROOT or SAXONC_HOME" OFF)

# SHA256 of official Saxonica SaxonC-HE 13.0.0 archives.
set(_CCCR_SAXON_CHE_SHA_linux_x86_64
    "5040efc0a113eb6aa3bd8a048a51cd9416dbc1844c1403b9b39bed72cfd2d3b7")
set(_CCCR_SAXON_CHE_SHA_linux_arm64
    "326b9bdc5df03bec6aa037d8a783b8efea02ffc7085a4995554943350c92523b")
set(_CCCR_SAXON_CHE_SHA_macos_x86_64
    "eb913720c669c263d74f869c55a526f87507a05adeae50892f704c4083145127")
set(_CCCR_SAXON_CHE_SHA_macos_arm64
    "dc5a184471e91d84030eb4a257b7c77c939e8913a05a78e8ba01575d2de08d8d")
set(_CCCR_SAXON_CHE_SHA_windows_x86_64
    "b7d2c63d77806d105b6f4b293fb5f7ea25f92853f206d0b7d5eb679ffa2dc2b3")

function(cccr_saxon_che_host_asset out_name out_url out_sha256)
  set(_ver_dashed "13-0-0")
  set(_base "https://downloads.saxonica.com/SaxonC/HE/13")

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
      set(_key "linux_x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
      set(_key "linux_arm64")
    else()
      message(FATAL_ERROR "SaxonC-HE: unsupported Linux arch '${CMAKE_SYSTEM_PROCESSOR}'")
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
      set(_key "macos_x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
      set(_key "macos_arm64")
    else()
      message(FATAL_ERROR "SaxonC-HE: unsupported macOS arch '${CMAKE_SYSTEM_PROCESSOR}'")
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_key "windows_x86_64")
  else()
    message(FATAL_ERROR "SaxonC-HE: unsupported OS '${CMAKE_SYSTEM_NAME}'")
  endif()

  if(_key STREQUAL "linux_x86_64")
    set(_name "SaxonCHE-linux-x86_64-${_ver_dashed}.zip")
  elseif(_key STREQUAL "linux_arm64")
    set(_name "SaxonCHE-linux-arm64-${_ver_dashed}.zip")
  elseif(_key STREQUAL "macos_x86_64")
    set(_name "SaxonCHE-macos-x86_64-${_ver_dashed}.zip")
  elseif(_key STREQUAL "macos_arm64")
    set(_name "SaxonCHE-macos-arm64-${_ver_dashed}.zip")
  elseif(_key STREQUAL "windows_x86_64")
    set(_name "SaxonCHE-windows-x86_64-${_ver_dashed}.zip")
  endif()

  set(_sha "${_CCCR_SAXON_CHE_SHA_${_key}}")

  set(${out_name} "${_name}" PARENT_SCOPE)
  set(${out_url} "${_base}/${_name}" PARENT_SCOPE)
  set(${out_sha256} "${_sha}" PARENT_SCOPE)
endfunction()

function(cccr_saxon_che_prepend_prefixes root)
  set(_che "${root}/SaxonCHE")
  set(_core "${root}/SaxonCCoreHE")
  if(NOT EXISTS "${_che}/lib/cmake/SaxonCHE/SaxonCHEConfig.cmake")
    message(FATAL_ERROR "SaxonC-HE: missing ${_che}/lib/cmake/SaxonCHE/SaxonCHEConfig.cmake")
  endif()
  if(NOT EXISTS "${_core}/lib/cmake/SaxonCCoreHE/SaxonCCoreHEConfig.cmake")
    message(FATAL_ERROR "SaxonC-HE: missing ${_core}/lib/cmake/SaxonCCoreHE/SaxonCCoreHEConfig.cmake")
  endif()

  # Point Config packages at the extract tree (avoids Conan dependency-provider noise).
  set(SaxonCHE_DIR "${_che}/lib/cmake/SaxonCHE" PARENT_SCOPE)
  set(SaxonCCoreHE_DIR "${_core}/lib/cmake/SaxonCCoreHE" PARENT_SCOPE)
  list(PREPEND CMAKE_PREFIX_PATH "${_che}" "${_core}")
  set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)

  set(CCCR_SAXON_CHE_ROOT "${root}" CACHE PATH "Resolved SaxonC-HE extract root" FORCE)

  # Build-tree RPATH dirs for final link artifacts (executables / shared libs).
  # Applied per-target via cccr_saxon_che_apply_build_rpath() — do not set
  # global CMAKE_BUILD_RPATH (breaks install-time RPATH_CHANGE rewriting).
  set(CCCR_SAXON_CHE_BUILD_RPATH
      "${_che}/lib"
      "${_core}/lib"
      CACHE INTERNAL "SaxonC-HE build-tree RPATH directories")
endfunction()

# Append SaxonC-HE lib dirs to a target's BUILD_RPATH (build-tree only).
function(cccr_saxon_che_apply_build_rpath target)
  if(NOT TARGET ${target})
    return()
  endif()
  if(NOT CCCR_SAXON_CHE_BUILD_RPATH)
    return()
  endif()
  set_property(TARGET ${target} APPEND PROPERTY
      BUILD_RPATH ${CCCR_SAXON_CHE_BUILD_RPATH})
endfunction()

function(cccr_saxon_che_configure_imported_targets)
  foreach(_cccr_saxon_tgt IN ITEMS SaxonCHE::saxonc-he SaxonCCoreHE::saxonc-core-he)
    if(TARGET ${_cccr_saxon_tgt})
      set_target_properties(${_cccr_saxon_tgt} PROPERTIES
          MAP_IMPORTED_CONFIG_DEBUG Release
          MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
          MAP_IMPORTED_CONFIG_MINSIZEREL Release
      )
    endif()
  endforeach()

  # libsaxonc-he.so NEEDED libsaxonc-core-he.so; DT_RUNPATH is not searched for
  # that transitive dependency. Prefer DT_RPATH for build-tree binaries.
  # Do not pass -rpath via INTERFACE link options — those bake absolute build
  # paths into binaries and fight install-time INSTALL_RPATH rewriting.
  if(NOT WIN32 AND NOT CCCR_SAXON_CHE_DTAGS_FIXED)
    add_link_options("LINKER:--disable-new-dtags")
    set(CCCR_SAXON_CHE_DTAGS_FIXED TRUE PARENT_SCOPE)
  endif()
endfunction()

function(cccr_find_or_fetch_saxon_che)
  if(TARGET SaxonCHE::saxonc-he)
    cccr_saxon_che_configure_imported_targets()
    return()
  endif()

  if(CCCR_SAXON_CHE_ROOT)
    cccr_saxon_che_prepend_prefixes("${CCCR_SAXON_CHE_ROOT}")
  elseif(DEFINED ENV{SAXONC_HOME} AND EXISTS "$ENV{SAXONC_HOME}/SaxonCHE")
    cccr_saxon_che_prepend_prefixes("$ENV{SAXONC_HOME}")
  endif()

  find_package(SaxonCHE ${CCCR_SAXON_CHE_VERSION} EXACT QUIET)
  if(TARGET SaxonCHE::saxonc-he)
    message(STATUS "SaxonC-HE ${CCCR_SAXON_CHE_VERSION}: found via find_package")
    cccr_saxon_che_configure_imported_targets()
    return()
  endif()

  if(CCCR_SAXON_OFFLINE)
    message(FATAL_ERROR "SaxonC-HE offline mode is ON: set CCCR_SAXON_CHE_ROOT to a pre-extracted SaxonC-HE root (contains SaxonCHE/ and SaxonCCoreHE/) or set SAXONC_HOME")
  endif()

  cccr_saxon_che_host_asset(_asset _url _sha256)
  if(NOT CCCR_CACHE_DIR)
    set(CCCR_CACHE_DIR "${PROJECT_SOURCE_DIR}/.cache")
  endif()
  set(_deps_dir "${CCCR_CACHE_DIR}/saxonc-he")
  set(_zip "${_deps_dir}/${_asset}")
  string(REGEX REPLACE "\\.zip$" "" _extract_name "${_asset}")
  set(_extract_root "${_deps_dir}/${_extract_name}")

  file(MAKE_DIRECTORY "${_deps_dir}")

  if(NOT EXISTS "${_extract_root}/SaxonCHE/lib/cmake/SaxonCHE/SaxonCHEConfig.cmake")
    message(STATUS "SaxonC-HE: downloading ${_url}")
    file(DOWNLOAD "${_url}" "${_zip}"
        EXPECTED_HASH "SHA256=${_sha256}"
        SHOW_PROGRESS
        STATUS _dl_status
        TLS_VERIFY ON)
    list(GET _dl_status 0 _dl_code)
    if(NOT _dl_code EQUAL 0)
      list(GET _dl_status 1 _dl_msg)
      message(FATAL_ERROR "SaxonC-HE download failed: ${_dl_msg} (${_url})")
    endif()

    message(STATUS "SaxonC-HE: extracting ${_asset}")
    file(ARCHIVE_EXTRACT INPUT "${_zip}" DESTINATION "${_deps_dir}")
  else()
    message(STATUS "SaxonC-HE: using cached ${_extract_root}")
  endif()

  cccr_saxon_che_prepend_prefixes("${_extract_root}")

  find_package(SaxonCHE ${CCCR_SAXON_CHE_VERSION} EXACT REQUIRED)
  if(NOT TARGET SaxonCHE::saxonc-he)
    message(FATAL_ERROR "SaxonC-HE: find_package succeeded but SaxonCHE::saxonc-he is missing")
  endif()
  message(STATUS "SaxonC-HE ${CCCR_SAXON_CHE_VERSION}: ready at ${CCCR_SAXON_CHE_ROOT}")
  cccr_saxon_che_configure_imported_targets()
endfunction()
