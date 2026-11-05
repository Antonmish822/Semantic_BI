set(CCCR_BINDINGS_C_HEADERS
    include/cccr/bindings/c/cccr.h
    include/cccr/bindings/c/export.h
    include/cccr/bindings/c/status.h
    include/cccr/bindings/c/context.h
    include/cccr/bindings/c/schema.h
    include/cccr/bindings/c/source.h
    include/cccr/bindings/c/graph.h
)

set(CCCR_BINDINGS_C_SOURCES
    src/status.cpp
    src/context.cpp
    src/schema.cpp
    src/source.cpp
    src/graph.cpp
)

set(CCCR_BINDINGS_C_ALL_FILES
    ${CCCR_BINDINGS_C_HEADERS}
    ${CCCR_BINDINGS_C_SOURCES}
)

# === Shared compiler optimization flags ===

add_library(cccr_bindings_c_compiler_optimizations INTERFACE)

target_compile_options(cccr_bindings_c_compiler_optimizations INTERFACE
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

add_library(cccr_bindings_c SHARED ${CCCR_BINDINGS_C_ALL_FILES})
add_library(cccr::bindings::c ALIAS cccr_bindings_c)

# PCH
target_precompile_headers(cccr_bindings_c PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hpp>
)

# C++ standart setup
set_target_properties(cccr_bindings_c PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# Output dir
set(_output_dir "${PROJECT_SOURCE_DIR}/bin/bindings/c/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${CMAKE_SYSTEM_PROCESSOR}>")
set_target_properties(cccr_bindings_c PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${_output_dir}"
  LIBRARY_OUTPUT_DIRECTORY "${_output_dir}"
  ARCHIVE_OUTPUT_DIRECTORY "${_output_dir}"
)

# Folder setup
set_target_properties(cccr_bindings_c PROPERTIES FOLDER bindings/c)

# Platform definitions
target_compile_definitions(cccr_bindings_c PUBLIC
    $<$<PLATFORM_ID:Windows>:CCCR_PLATFORM_WINDOWS>
    $<$<PLATFORM_ID:Linux>:CCCR_PLATFORM_LINUX>
    $<$<PLATFORM_ID:Darwin>:CCCR_PLATFORM_MACOS>
    $<$<CONFIG:Debug>:CCCR_DEBUG_MODE>
    $<$<CONFIG:RelWithDebInfo>:CCCR_RELEASE_WITH_DEBUG_INFO_MODE>
    $<$<CONFIG:Release>:CCCR_RELEASE_MODE NDEBUG>
)

target_compile_definitions(cccr_bindings_c PRIVATE
    CCCR_BINDINGS_C_BUILDING
    CCCR_PROJECT_VERSION="${PROJECT_VERSION}"
)

if(WIN32)
  target_compile_definitions(cccr_bindings_c PUBLIC
      UNICODE _UNICODE
      WIN32_LEAN_AND_MEAN
      NOMINMAX
  )
endif()

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

target_compile_options(cccr_bindings_c PRIVATE
    # MSVC and clang-cl (MSVC frontend) use MSVC-style warnings
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:${MSVC_WARNINGS}>
    # Clang on Unix-like systems
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<PLATFORM_ID:Windows>>>:${CLANG_WARNINGS}>
    # GCC
    $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNINGS}>
)

# Optimization flags (shared)
target_link_libraries(cccr_bindings_c PRIVATE
    $<BUILD_INTERFACE:cccr_bindings_c_compiler_optimizations>
)

# MSVC Runtime
if(WIN32 AND MSVC)
  set_target_properties(${TARGET} PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
  )
endif()

# PIC
set_target_properties(cccr_bindings_c PROPERTIES POSITION_INDEPENDENT_CODE ON)

# LTO
if(CCCR_IPO_SUPPORTED)
  set_target_properties(cccr_bindings_c PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
      INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
  )
endif()

# Sanitizer support
cccr_target_enable_sanitizers(cccr_bindings_c)

target_include_directories(cccr_bindings_c
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cccr_bindings_c PRIVATE
    cccr::bindings::common
)

cccr_saxon_che_apply_build_rpath(cccr_bindings_c)

if(CCCR_BUILD_TESTS)
  set(CCCR_BINDINGS_C_TESTS
      tests/main.cpp
  )

  add_executable(cccr_bindings_c_tests ${CCCR_BINDINGS_C_TESTS})

  # C++ standart setup
  set_target_properties(cccr_bindings_c_tests PROPERTIES
      CXX_STANDARD 23
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
  )

  # Output dir
  set(_output_dir "${PROJECT_SOURCE_DIR}/bin/tests/bindings/c/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${CMAKE_SYSTEM_PROCESSOR}>")
  set_target_properties(cccr_bindings_c_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_output_dir}"
    LIBRARY_OUTPUT_DIRECTORY "${_output_dir}"
    ARCHIVE_OUTPUT_DIRECTORY "${_output_dir}"
  )

  # Folder setup
  set_target_properties(cccr_bindings_c_tests PROPERTIES FOLDER bindings/c/tests)

  # Optimization flags only
  target_link_libraries(cccr_bindings_c_tests PRIVATE cccr_bindings_c_compiler_optimizations)

  # LTO
  if(CCCR_IPO_SUPPORTED)
    set_target_properties(cccr_bindings_c_tests PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
        INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    )
  endif()

  # Sanitizer support
  cccr_target_enable_sanitizers(cccr_bindings_c_tests)

  target_compile_definitions(cccr_bindings_c_tests PRIVATE
      CCCR_BINDINGS_BASE_DATA_DIR="${PROJECT_SOURCE_DIR}/examples/data/base"
  )

  target_link_libraries(cccr_bindings_c_tests PRIVATE
      cccr::bindings::c
      doctest::doctest
  )

  cccr_saxon_che_apply_build_rpath(cccr_bindings_c_tests)

  add_test(NAME cccr_bindings_c_tests COMMAND cccr_bindings_c_tests)
  set_property(TEST cccr_bindings_c_tests PROPERTY LABELS "unit" "bindings" "bindings_c")
endif()
