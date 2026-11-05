include_guard(GLOBAL)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Opt-in packaging (CCCR_ENABLE_INSTALL). Safe to leave OFF while the public API
# is unstable. When enabled, installs the libraries/tools that exist in this
# build and redistributes the SaxonC-HE runtime that cccr_core links against.

function(cccr_install_set_runtime_rpath target)
  # Installed layout: bindir/tool → ../lib/libsaxonc-*.so
  #                    libdir/libcccr_*.so → ./libsaxonc-*.so
  get_target_property(_type ${target} TYPE)
  if(_type STREQUAL "EXECUTABLE")
    set_target_properties(${target} PROPERTIES
        INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH FALSE
        BUILD_WITH_INSTALL_RPATH FALSE
    )
  elseif(_type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "MODULE_LIBRARY")
    set_target_properties(${target} PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        INSTALL_RPATH_USE_LINK_PATH FALSE
        BUILD_WITH_INSTALL_RPATH FALSE
    )
  endif()
endfunction()

function(cccr_configure_install)
  if(NOT TARGET cccr_core)
    message(FATAL_ERROR "cccr_configure_install: cccr_core target is missing")
  endif()

  set_target_properties(cccr_core PROPERTIES EXPORT_NAME core)

  set(_export_targets cccr_core)
  set(_runtime_targets)

  if(TARGET cccr_bindings_c)
    # Nested export name → imported target cccr::bindings::c (NAMESPACE cccr::).
    set_target_properties(cccr_bindings_c PROPERTIES EXPORT_NAME bindings::c)
    list(APPEND _export_targets cccr_bindings_c)
    cccr_install_set_runtime_rpath(cccr_bindings_c)
  endif()

  if(TARGET cccr_cli)
    list(APPEND _runtime_targets cccr_cli)
    cccr_install_set_runtime_rpath(cccr_cli)
  endif()

  # Example executables (optional build) — install next to CLI with same RPATH.
  foreach(_ex IN ITEMS
      cccr_example_primer cccr_example_kug cccr_example_raspisanie)
    if(TARGET ${_ex})
      list(APPEND _runtime_targets ${_ex})
      cccr_install_set_runtime_rpath(${_ex})
    endif()
  endforeach()

  install(TARGETS ${_export_targets}
      EXPORT CCCRTargets
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
      INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )

  if(_runtime_targets)
    install(TARGETS ${_runtime_targets}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
  endif()

  # Public headers (core always; C bindings when built).
  # Python is packaged via scikit-build (SKBUILD), not this export.
  set(_header_dirs src/core/include)
  if(TARGET cccr_bindings_c)
    list(APPEND _header_dirs src/bindings/c/include)
  endif()
  foreach(_header_dir IN LISTS _header_dirs)
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/${_header_dir}/"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
    )
  endforeach()

  # Redistribute SaxonC-HE shared libs + CMake packages into the install prefix
  # so find_dependency(SaxonCHE) and runtime $ORIGIN RPATH both resolve:
  #   <prefix>/lib/libsaxonc-he.so*
  #   <prefix>/lib/libsaxonc-core-he.so*
  #   <prefix>/lib/cmake/SaxonCHE/
  #   <prefix>/lib/cmake/SaxonCCoreHE/
  if(CCCR_SAXON_CHE_ROOT)
    set(_saxon_libs_installed FALSE)
    foreach(_saxon_pkg IN ITEMS SaxonCHE SaxonCCoreHE)
      set(_saxon_root "${CCCR_SAXON_CHE_ROOT}/${_saxon_pkg}")
      if(EXISTS "${_saxon_root}/lib")
        # Shared libraries — DIRECTORY + PATTERN keeps SONAME symlink chains.
        file(GLOB _saxon_sos "${_saxon_root}/lib/libsaxonc*.so*")
        if(_saxon_sos)
          install(DIRECTORY "${_saxon_root}/lib/"
              DESTINATION "${CMAKE_INSTALL_LIBDIR}"
              USE_SOURCE_PERMISSIONS
              FILES_MATCHING
                  PATTERN "libsaxonc*.so"
                  PATTERN "libsaxonc*.so.*"
          )
          set(_saxon_libs_installed TRUE)
        endif()

        # Upstream CMake packages (paths use ${_IMPORT_PREFIX}/lib/...).
        if(EXISTS "${_saxon_root}/lib/cmake")
          install(DIRECTORY "${_saxon_root}/lib/cmake/"
              DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake"
              USE_SOURCE_PERMISSIONS
          )
        endif()
      endif()
      if(EXISTS "${_saxon_root}/include")
        install(DIRECTORY "${_saxon_root}/include/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )
      endif()
    endforeach()

    if(NOT _saxon_libs_installed)
      message(WARNING
        "cccr_configure_install: no libsaxonc*.so under ${CCCR_SAXON_CHE_ROOT}; "
        "installed binaries will fail to load SaxonC-HE at runtime.")
    else()
      message(STATUS
        "Install: SaxonC-HE shared libs → \${CMAKE_INSTALL_PREFIX}/"
        "${CMAKE_INSTALL_LIBDIR}")
    endif()
  else()
    message(WARNING "cccr_configure_install: CCCR_SAXON_CHE_ROOT is empty; "
                    "SaxonC-HE runtime will not be installed. Consumers must "
                    "provide SaxonCHE themselves.")
  endif()

  # Vendored static Redland archives (needed by consumers of static cccr_core).
  if(CCCR_REDLAND_VENDORED AND CCCR_REDLAND_LINKED_STATIC AND CCCR_REDLAND_PREFIX)
    foreach(_archive IN ITEMS libraptor2.a librasqal.a librdf.a)
      if(EXISTS "${CCCR_REDLAND_PREFIX}/lib/${_archive}")
        install(FILES "${CCCR_REDLAND_PREFIX}/lib/${_archive}"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        )
      endif()
    endforeach()
    foreach(_inc IN ITEMS
        "${CCCR_REDLAND_PREFIX}/include/redland.h"
        "${CCCR_REDLAND_PREFIX}/include/librdf.h")
      if(EXISTS "${_inc}")
        install(FILES "${_inc}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
      endif()
    endforeach()
    foreach(_incdir IN ITEMS raptor2 rasqal)
      if(EXISTS "${CCCR_REDLAND_PREFIX}/include/${_incdir}")
        install(DIRECTORY "${CCCR_REDLAND_PREFIX}/include/${_incdir}/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${_incdir}"
        )
      endif()
    endforeach()
    message(STATUS
      "Install: vendored static Redland archives → \${CMAKE_INSTALL_PREFIX}/"
      "${CMAKE_INSTALL_LIBDIR}")
  endif()

  set(_package_dir "${CMAKE_INSTALL_LIBDIR}/cmake/CCCR")

  set(CCCR_CONFIG_SAXON_CHE_VERSION ${CCCR_SAXON_CHE_VERSION})
  set(CCCR_CONFIG_REDLAND_VENDORED ${CCCR_REDLAND_VENDORED})
  set(CCCR_CONFIG_REDLAND_STATIC ${CCCR_REDLAND_LINKED_STATIC})

  configure_package_config_file(
      "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/CCCRConfig.cmake.in"
      "${PROJECT_BINARY_DIR}/CCCRConfig.cmake"
      INSTALL_DESTINATION "${_package_dir}"
  )
  write_basic_package_version_file(
      "${PROJECT_BINARY_DIR}/CCCRConfigVersion.cmake"
      VERSION "${PROJECT_VERSION}"
      COMPATIBILITY SameMajorVersion
  )

  install(EXPORT CCCRTargets
      FILE CCCRTargets.cmake
      NAMESPACE cccr::
      DESTINATION "${_package_dir}"
  )
  install(FILES
      "${PROJECT_BINARY_DIR}/CCCRConfig.cmake"
      "${PROJECT_BINARY_DIR}/CCCRConfigVersion.cmake"
      DESTINATION "${_package_dir}"
  )
endfunction()
