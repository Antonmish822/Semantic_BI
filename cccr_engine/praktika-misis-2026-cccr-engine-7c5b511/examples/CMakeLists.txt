# CCCR engine usage examples (one executable per domain sample).

set(MSVC_WARNINGS
    /W4
    /w14242 /w14254 /w14263 /w14265 /w14287 /we4289 /w14296
    /w14311 /w14545 /w14546 /w14547 /w14549 /w14555
    /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
)

set(CLANG_WARNINGS
    -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wold-style-cast
    -Wcast-align -Wunused -Woverloaded-virtual -Wconversion -Wsign-conversion
    -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough
)

set(GCC_WARNINGS
    ${CLANG_WARNINGS}
    -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches
    -Wlogical-op -Wuseless-cast
)

if(CCCR_ENABLE_WARNINGS_AS_ERRORS)
  list(APPEND MSVC_WARNINGS /WX)
  list(APPEND CLANG_WARNINGS -Werror)
  list(APPEND GCC_WARNINGS -Werror)
endif()

add_library(cccr_examples_compiler_warnings INTERFACE)
target_compile_options(cccr_examples_compiler_warnings INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:${MSVC_WARNINGS}>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<PLATFORM_ID:Windows>>>:${CLANG_WARNINGS}>
    $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNINGS}>
)

add_library(cccr_examples_compiler_optimizations INTERFACE)
target_compile_options(cccr_examples_compiler_optimizations INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
        /Zc:preprocessor
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /DNDEBUG>
        $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG>
    >
    $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<NOT:$<PLATFORM_ID:Windows>>>:
        $<$<CONFIG:Debug>:-O0 -g3 -ggdb>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
        $<$<CONFIG:Release>:-O3 -DNDEBUG>
    >
)

set(CCCR_EXAMPLE_NAMES
    base
    kug
    schedule
)

foreach(_name IN LISTS CCCR_EXAMPLE_NAMES)
  set(_target "cccr_example_${_name}")
  add_executable(${_target} ${_name}.cpp)

  set_target_properties(${_target} PROPERTIES
      CXX_STANDARD 23
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
      FOLDER examples
  )

  set(_output_dir "${PROJECT_SOURCE_DIR}/bin/examples/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${CMAKE_SYSTEM_PROCESSOR}>")
  set_target_properties(${_target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${_output_dir}"
      LIBRARY_OUTPUT_DIRECTORY "${_output_dir}"
      ARCHIVE_OUTPUT_DIRECTORY "${_output_dir}"
  )

  target_include_directories(${_target} PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}"
  )

  target_compile_definitions(${_target} PRIVATE
      CCCR_EXAMPLE_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/${_name}"
  )

  target_link_libraries(${_target} PRIVATE
      cccr::core
      cccr_examples_compiler_warnings
      cccr_examples_compiler_optimizations
  )

  cccr_saxon_che_apply_build_rpath(${_target})

  if(CCCR_IPO_SUPPORTED)
    set_target_properties(${_target} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
        INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    )
  endif()

  cccr_target_enable_sanitizers(${_target})
endforeach()
