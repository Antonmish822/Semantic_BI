set(CCCR_BINDINGS_PYTHON_SOURCES
    src/module.cpp
    src/graph.cpp
    src/context.cpp
    src/bind_schema.cpp
    src/bind_source.cpp
    src/bind_graph.cpp
    src/bind_context.cpp
)

pybind11_add_module(cccr_py MODULE ${CCCR_BINDINGS_PYTHON_SOURCES})
add_library(cccr::bindings::python ALIAS cccr_py)

# PCH
target_precompile_headers(cccr_py PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp>
)

# C++ standart setup
set_target_properties(cccr_py PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Output dir
set(_output_dir "${PROJECT_SOURCE_DIR}/bin/bindings/python/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${CMAKE_SYSTEM_PROCESSOR}>")
set_target_properties(cccr_py PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${_output_dir}"
  LIBRARY_OUTPUT_DIRECTORY "${_output_dir}"
  ARCHIVE_OUTPUT_DIRECTORY "${_output_dir}"
)

# Folder setup
set_target_properties(cccr_py PROPERTIES FOLDER bindings/python)

# Loaded into a non-instrumented Python process — must not use ASan.
set_target_properties(cccr_py PROPERTIES CCCR_NO_SANITIZE TRUE)

# Warnings
set(MSVC_WARNINGS
    /W4              # High warning level
    /w14242          # Conversion warnings
    /w14254          # Operator conversion
    /w14263          # Member function doesn't override
    /w14265          # Virtual destructor warning
    /w14287          # Unsigned/negative constant mismatch
    /we4289          # Loop variable used outside scope
    /w14296          # Expression is always true/false
    /w14311          # Pointer truncation
    /w14545 /w14546 /w14547 /w14549 /w14555 # Suspicious operations
    /w14619          # Unknown warning number
    /w14640          # Thread-unsafe static initialization
    /w14826          # Wide character conversion
    /w14905 /w14906  # String literal casts
    /w14928          # Illegal copy initialization
)

set(CLANG_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wimplicit-fallthrough
)

set(GCC_WARNINGS
    ${CLANG_WARNINGS}
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
)

if(CCCR_ENABLE_WARNINGS_AS_ERRORS)
  list(APPEND MSVC_WARNINGS /WX)
  list(APPEND CLANG_WARNINGS -Werror)
  list(APPEND GCC_WARNINGS -Werror)
endif()

target_compile_options(cccr_py PRIVATE
    # MSVC and clang-cl (MSVC frontend) use MSVC-style warnings
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:${MSVC_WARNINGS}>
    # Clang on Unix-like systems
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<PLATFORM_ID:Windows>>>:${CLANG_WARNINGS}>
    # GCC
    $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNINGS}>
)

# Optimization flags
target_compile_options(cccr_py PRIVATE
    # MSVC and clang-cl (MSVC frontend)
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
        /Zc:preprocessor
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /DNDEBUG>
        $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG>
    >
    # GCC and Clang on Unix-like systems
    $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<NOT:$<PLATFORM_ID:Windows>>>:
        $<$<CONFIG:Debug>:-O0 -g3 -ggdb>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    >
)

# MSVC Runtime
if(WIN32 AND MSVC)
  set_target_properties(cccr_py PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
  )
endif()

# PIC
set_target_properties(cccr_py PROPERTIES POSITION_INDEPENDENT_CODE ON)

# LTO
if(CCCR_IPO_SUPPORTED)
  set_target_properties(cccr_py PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
      INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
  )
endif()

# Sanitizer support is intentionally skipped: this MODULE is loaded by a
# non-instrumented Python interpreter (see CCCR_NO_SANITIZE above).

target_compile_definitions(cccr_py PRIVATE
    CCCR_VERSION_INFO="${PROJECT_VERSION}"
)

target_include_directories(cccr_py PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cccr_py PRIVATE
    cccr::bindings::common
)

cccr_saxon_che_apply_build_rpath(cccr_py)

if(CCCR_BUILD_TESTS)
  if(NOT DEFINED Python_EXECUTABLE)
    if(DEFINED PYTHON_EXECUTABLE)
      set(_cccr_python_exe "${PYTHON_EXECUTABLE}")
    else()
      find_program(_cccr_python_exe NAMES python3 python REQUIRED)
    endif()
  else()
    set(_cccr_python_exe "${Python_EXECUTABLE}")
  endif()

  set(_python_module_dir
      "${PROJECT_SOURCE_DIR}/bin/bindings/python/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${CMAKE_SYSTEM_PROCESSOR}>")
  add_test(
      NAME cccr_py_smoke
      COMMAND
          ${_cccr_python_exe}
          "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.py"
          "${PROJECT_SOURCE_DIR}/examples/data/base"
  )
  set_tests_properties(cccr_py_smoke PROPERTIES
      LABELS "unit;bindings;bindings_python"
      ENVIRONMENT "PYTHONPATH=${_python_module_dir}"
      DEPENDS cccr_py
  )
endif()

if(SKBUILD)
  # Top-level extension: `import cccr_py` (not a package sub-module).
  install(TARGETS cccr_py DESTINATION . COMPONENT python)
  # Keep Saxon loadable after wheel install when the build RPATH is rewritten.
  if(CCCR_SAXON_CHE_BUILD_RPATH)
    set_property(TARGET cccr_py APPEND PROPERTY
        INSTALL_RPATH ${CCCR_SAXON_CHE_BUILD_RPATH})
  endif()
endif()
