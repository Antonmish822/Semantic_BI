# ============================================================================
# Sanitizer Configuration for cccr
# ============================================================================
#
# This module provides support for various C++ sanitizers:
# - AddressSanitizer (ASan): Detects memory errors (buffer overflows, use-after-free, etc.)
# - UndefinedBehaviorSanitizer (UBSan): Detects undefined behavior
# - ThreadSanitizer (TSan): Detects data races (mutually exclusive with ASan)
# - MemorySanitizer (MSan): Detects uninitialized memory reads (Clang only, requires instrumented libc++)
#
# Usage:
#   include(Sanitizers)
#   cccr_target_enable_sanitizers(my_target)
#
# Options:
#   CCCR_DEVELOPER_MODE          - Expose sanitizer options and functionality (default: OFF)
#   CCCR_ENABLE_SANITIZERS       - Master switch for sanitizers (default: ON for Debug)
#   CCCR_SANITIZER_ADDRESS       - Enable AddressSanitizer (default: ON)
#   CCCR_SANITIZER_UNDEFINED     - Enable UndefinedBehaviorSanitizer (default: ON)
#   CCCR_SANITIZER_THREAD        - Enable ThreadSanitizer (default: OFF, mutually exclusive with ASan)
#   CCCR_SANITIZER_MEMORY        - Enable MemorySanitizer (default: OFF, Clang only)
#
# Notes:
#   - ASan and TSan cannot be used together
#   - MSan requires the entire program (including libc++) to be built with MSan
#   - MSVC only supports ASan (/fsanitize=address)
#   - Sanitizers are typically only enabled for Debug builds
#
# ============================================================================

include_guard(GLOBAL)

# Sanitizers are a developer-only feature.
if(NOT CCCR_DEVELOPER_MODE)
  #[[
      cccr_target_enable_sanitizers(<target>)

      Developer-mode sanitizer hook. When CCCR_DEVELOPER_MODE is OFF this is
      a no-op so targets can call it unconditionally.

      Example:
          cccr_target_enable_sanitizers(cccr_module_core)
  ]]
  function(cccr_target_enable_sanitizers TARGET)
  endfunction()

  #[[
      cccr_print_sanitizer_status()

      Prints the sanitizer configuration summary. When developer mode is OFF it
      reports that sanitizer support is disabled.

      Example:
          cccr_print_sanitizer_status()
  ]]
  function(cccr_print_sanitizer_status)
    message(STATUS "Sanitizers: DISABLED (CCCR_DEVELOPER_MODE=OFF)")
  endfunction()

  return()
endif()

# Detect compiler capabilities
set(CCCR_COMPILER_IS_GNU OFF)
set(CCCR_COMPILER_IS_CLANG OFF)
set(CCCR_COMPILER_IS_MSVC OFF)
set(CCCR_COMPILER_IS_CLANG_CL OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CCCR_COMPILER_IS_GNU ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(CCCR_COMPILER_IS_CLANG_CL ON)
  else()
    set(CCCR_COMPILER_IS_CLANG ON)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(CCCR_COMPILER_IS_MSVC ON)
endif()

# ============================================================================
# Sanitizer Options
# ============================================================================

# Master switch - enabled by default for Debug builds
option(CCCR_ENABLE_SANITIZERS "Enable sanitizers for Debug builds" ON)

# Individual sanitizer options
option(CCCR_SANITIZER_ADDRESS "Enable AddressSanitizer" ON)
option(CCCR_SANITIZER_UNDEFINED "Enable UndefinedBehaviorSanitizer" ON)
option(CCCR_SANITIZER_THREAD "Enable ThreadSanitizer (mutually exclusive with ASan)" OFF)
option(CCCR_SANITIZER_MEMORY "Enable MemorySanitizer (Clang only, requires instrumented libc++)" OFF)

# ============================================================================
# Validation
# ============================================================================

# Check for mutually exclusive sanitizers
if(CCCR_SANITIZER_ADDRESS AND CCCR_SANITIZER_THREAD)
  message(WARNING "AddressSanitizer and ThreadSanitizer cannot be used together. Disabling ThreadSanitizer.")
  set(CCCR_SANITIZER_THREAD OFF CACHE BOOL "Enable ThreadSanitizer" FORCE)
endif()

if(CCCR_SANITIZER_ADDRESS AND CCCR_SANITIZER_MEMORY)
  message(WARNING "AddressSanitizer and MemorySanitizer cannot be used together. Disabling MemorySanitizer.")
  set(CCCR_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

if(CCCR_SANITIZER_THREAD AND CCCR_SANITIZER_MEMORY)
  message(WARNING "ThreadSanitizer and MemorySanitizer cannot be used together. Disabling MemorySanitizer.")
  set(CCCR_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

# MSan is only available on Clang
if(CCCR_SANITIZER_MEMORY AND NOT CCCR_COMPILER_IS_CLANG)
  message(WARNING "MemorySanitizer is only available with Clang. Disabling MemorySanitizer.")
  set(CCCR_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

# MSVC only supports ASan
if(CCCR_COMPILER_IS_MSVC OR CCCR_COMPILER_IS_CLANG_CL)
  if(CCCR_SANITIZER_UNDEFINED)
    message(STATUS "UndefinedBehaviorSanitizer is not supported on MSVC. Disabling.")
    set(CCCR_SANITIZER_UNDEFINED OFF CACHE BOOL "Enable UndefinedBehaviorSanitizer" FORCE)
  endif()
  if(CCCR_SANITIZER_THREAD)
    message(STATUS "ThreadSanitizer is not supported on MSVC. Disabling.")
    set(CCCR_SANITIZER_THREAD OFF CACHE BOOL "Enable ThreadSanitizer" FORCE)
  endif()
  if(CCCR_SANITIZER_MEMORY)
    message(STATUS "MemorySanitizer is not supported on MSVC. Disabling.")
    set(CCCR_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
  endif()
endif()

# ============================================================================
# Global Sanitizer Flags (MSVC / clang-cl)
# ============================================================================

# MSVC ASAN changes the ABI of STL types (std::string, std::vector,
# std::optional, etc.). All translation units linked into the same binary
# must be compiled with the same /fsanitize=address flag, otherwise the
# linker rejects the object files. Apply globally so dependencies also
# receive the flag. Targets may opt out via property CCCR_NO_SANITIZE
# (required for Python modules loaded into a non-instrumented interpreter).
if(CCCR_COMPILER_IS_MSVC AND CCCR_ENABLE_SANITIZERS AND CCCR_SANITIZER_ADDRESS)
  add_compile_options(
    "$<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:$<TARGET_PROPERTY:CCCR_NO_SANITIZE>>>>:/fsanitize=address>"
  )
elseif(CCCR_COMPILER_IS_CLANG_CL AND CCCR_ENABLE_SANITIZERS AND CCCR_SANITIZER_ADDRESS)
  add_compile_options(
    "$<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:$<TARGET_PROPERTY:CCCR_NO_SANITIZE>>>>:-fsanitize=address>"
  )
endif()

# ============================================================================
# Internal Helper Functions
# ============================================================================

# Build the sanitizer flags string for GCC/Clang
function(_cccr_get_sanitizer_flags OUT_COMPILE_FLAGS OUT_LINK_FLAGS)
  set(_compile_flags "")
  set(_link_flags "")

  if(CCCR_COMPILER_IS_GNU OR CCCR_COMPILER_IS_CLANG)
    set(_sanitizers "")

    if(CCCR_SANITIZER_ADDRESS)
      list(APPEND _sanitizers "address")
    endif()

    if(CCCR_SANITIZER_UNDEFINED)
      list(APPEND _sanitizers "undefined")
    endif()

    if(CCCR_SANITIZER_THREAD)
      list(APPEND _sanitizers "thread")
    endif()

    if(CCCR_SANITIZER_MEMORY)
      list(APPEND _sanitizers "memory")
    endif()

    if(_sanitizers)
      list(JOIN _sanitizers "," _sanitizer_list)
      set(_compile_flags "-fsanitize=${_sanitizer_list} -fno-omit-frame-pointer -fno-optimize-sibling-calls")
      set(_link_flags "-fsanitize=${_sanitizer_list}")

      # Add extra flags for better error reporting
      if(CCCR_SANITIZER_ADDRESS)
        string(APPEND _compile_flags " -fsanitize-address-use-after-scope")
      endif()

      if(CCCR_SANITIZER_UNDEFINED)
        # Print stack trace on UBSan error
        string(APPEND _compile_flags " -fno-sanitize-recover=undefined")
      endif()
    endif()
  endif()

  set(${OUT_COMPILE_FLAGS} "${_compile_flags}" PARENT_SCOPE)
  set(${OUT_LINK_FLAGS} "${_link_flags}" PARENT_SCOPE)
endfunction()

# ============================================================================
# Global Sanitizer Flags (GCC / Clang)
# ============================================================================
#
# Static libraries embed instrumented .o files. Every executable (and shared
# library) that links them must also link the sanitizer runtime — PRIVATE
# link options on static targets do not propagate to consumers.

if((CCCR_COMPILER_IS_GNU OR CCCR_COMPILER_IS_CLANG) AND CCCR_ENABLE_SANITIZERS)
  _cccr_get_sanitizer_flags(_cccr_global_sanitizer_compile_flags _cccr_global_sanitizer_link_flags)

  if(_cccr_global_sanitizer_compile_flags)
    separate_arguments(_cccr_global_sanitizer_compile_flags_list
        UNIX_COMMAND "${_cccr_global_sanitizer_compile_flags}")
    foreach(_cccr_sanitizer_compile_flag IN LISTS _cccr_global_sanitizer_compile_flags_list)
      add_compile_options(
        "$<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:$<TARGET_PROPERTY:CCCR_NO_SANITIZE>>>>:${_cccr_sanitizer_compile_flag}>"
      )
    endforeach()
  endif()

  if(_cccr_global_sanitizer_link_flags)
    add_link_options(
      "$<$<AND:$<CONFIG:Debug>,$<NOT:$<BOOL:$<TARGET_PROPERTY:CCCR_NO_SANITIZE>>>>:SHELL:${_cccr_global_sanitizer_link_flags}>"
    )
  endif()
endif()

# ============================================================================
# Public API
# ============================================================================

#[[
    cccr_target_enable_sanitizers(<target>)

    Applies sanitizer instrumentation to a target when sanitizer policy is not
    already handled globally for the active compiler.
]]
function(cccr_target_enable_sanitizers TARGET)
  if(NOT CCCR_ENABLE_SANITIZERS)
    return()
  endif()

  # Sanitizers are applied globally (see sections above) so instrumentation
  # and runtime linking stay consistent across static libs and executables.
  if(CCCR_COMPILER_IS_MSVC OR CCCR_COMPILER_IS_CLANG_CL
      OR CCCR_COMPILER_IS_GNU OR CCCR_COMPILER_IS_CLANG)
    return()
  endif()

  # Only apply sanitizers for Debug builds
  set(_is_debug "$<CONFIG:Debug>")

  _cccr_get_sanitizer_flags(_compile_flags _link_flags)

  if(_compile_flags)
    separate_arguments(_compile_flags_list UNIX_COMMAND "${_compile_flags}")
    foreach(_cccr_sanitizer_compile_flag IN LISTS _compile_flags_list)
      target_compile_options(${TARGET} PRIVATE
              "$<$<CONFIG:Debug>:${_cccr_sanitizer_compile_flag}>")
    endforeach()
  endif()

  if(_link_flags)
    target_link_options(${TARGET} PRIVATE
            "$<$<CONFIG:Debug>:SHELL:${_link_flags}>")
  endif()
endfunction()

#[[
    cccr_print_sanitizer_status()

    Prints the active sanitizer configuration.
]]
function(cccr_print_sanitizer_status)
  if(NOT CCCR_ENABLE_SANITIZERS)
    message(STATUS "Sanitizers: DISABLED")
    return()
  endif()

  message(STATUS "")
  message(STATUS "========== Sanitizer Configuration ==========")
  message(STATUS "Sanitizers enabled for Debug builds")

  if(CCCR_COMPILER_IS_MSVC OR CCCR_COMPILER_IS_CLANG_CL)
    message(STATUS "  Compiler: MSVC/clang-cl (limited sanitizer support)")
    if(CCCR_SANITIZER_ADDRESS)
      message(STATUS "  ✓ AddressSanitizer")
    endif()
  else()
    message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
    if(CCCR_SANITIZER_ADDRESS)
      message(STATUS "  ✓ AddressSanitizer")
    endif()
    if(CCCR_SANITIZER_UNDEFINED)
      message(STATUS "  ✓ UndefinedBehaviorSanitizer")
    endif()
    if(CCCR_SANITIZER_THREAD)
      message(STATUS "  ✓ ThreadSanitizer")
    endif()
    if(CCCR_SANITIZER_MEMORY)
      message(STATUS "  ✓ MemorySanitizer")
    endif()
  endif()

  message(STATUS "==============================================")
  message(STATUS "")
endfunction()
