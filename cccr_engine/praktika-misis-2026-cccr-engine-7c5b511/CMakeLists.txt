cmake_minimum_required(VERSION 3.25)

# CMake 4.0+ compat
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version" FORCE)
endif()

project (cccr-engine
    VERSION 0.1.0
    DESCRIPTION "CCCR engine"
    LANGUAGES C CXX
)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
  message(STATUS "Setting build type to 'Release' as none was specified.")
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "" FORCE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Workaround: disable CMake C++20 module scanning and PCH to avoid GCC mapper errors
# on non-ASCII source paths (e.g., 'ПроектыПитон') and Python wheel builds via scikit-build-core.
# See: https://gitlab.kitware.com/cmake/cmake/-/issues (similar reports for INCLUDE-TRANSLATE)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)

option(CCCR_DEVELOPER_MODE "Enable developer-focused build features" OFF)
if(DEFINED DEVELOPER_MODE)
  set(CCCR_DEVELOPER_MODE ${DEVELOPER_MODE} CACHE BOOL "Enable developer-focused build features" FORCE)
endif()

option(CCCR_BUILD_CLI "Build CLI" ${PROJECT_IS_TOP_LEVEL})
option(CCCR_BUILD_BINDINGS_C "Build C bindings" ${PROJECT_IS_TOP_LEVEL})
option(CCCR_BUILD_BINDINGS_PYTHON "Build Python bindings" ${PROJECT_IS_TOP_LEVEL})

set(_CCCR_BUILD_TESTS_DEFAULT ${PROJECT_IS_TOP_LEVEL})
if(DEFINED BUILD_TESTING AND NOT DEFINED CCCR_BUILD_TESTS)
  set(_CCCR_BUILD_TESTS_DEFAULT ${BUILD_TESTING})
endif()
option(CCCR_BUILD_TESTS "Build tests" ${_CCCR_BUILD_TESTS_DEFAULT})
set(BUILD_TESTING ${CCCR_BUILD_TESTS} CACHE BOOL "Build tests" FORCE)
unset(_CCCR_BUILD_TESTS_DEFAULT)

option(CCCR_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})

option(CCCR_ENABLE_LTO "Enable Link Time Optimization" ON)
option(CCCR_ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(CCCR_ENABLE_INSTALL "Enable installation rules" OFF)

# Include utilities
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers")

include(CTest)
enable_testing()

# Configure Clang archiver for LTO support
include(ClangArchiver)
cccr_configure_clang_archiver()

if(CCCR_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT CCCR_IPO_SUPPORTED OUTPUT error)
  if(CCCR_IPO_SUPPORTED)
    set(CCCR_IPO_SUPPORTED "${CCCR_IPO_SUPPORTED}" CACHE INTERNAL "IPO/LTO support result")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "Link-Time Optimization (LTO) enabled for Release and RelWithDebInfo builds")
  else()
    message(STATUS "IPO/LTO not supported: ${error}")
    set(CCCR_ENABLE_LTO OFF)
  endif()
  set(CCCR_IPO_CACHED TRUE CACHE INTERNAL "IPO/LTO support has been checked globally")
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled for multi-config generators")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled on Linux")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled for ${CMAKE_BUILD_TYPE} builds")
endif()

# Configure sanitizers
include(Sanitizers)
cccr_print_sanitizer_status()

# Third-party libraries (vendored under third-party/ + FindOrFetch SaxonC-HE)
include(Dependencies)

add_subdirectory(src/core)

if(CCCR_BUILD_CLI)
  add_subdirectory(src/cli)
endif()

if(CCCR_BUILD_BINDINGS_C OR CCCR_BUILD_BINDINGS_PYTHON)
  add_subdirectory(src/bindings/common)
endif()

if(CCCR_BUILD_BINDINGS_C)
  add_subdirectory(src/bindings/c)
endif()

if(CCCR_BUILD_BINDINGS_PYTHON)
  add_subdirectory(src/bindings/python)
endif()

if(CCCR_BUILD_TESTS)
  add_subdirectory(tests)
endif()

if(CCCR_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(CCCR_ENABLE_INSTALL)
  include(Install)
  cccr_configure_install()
endif()
