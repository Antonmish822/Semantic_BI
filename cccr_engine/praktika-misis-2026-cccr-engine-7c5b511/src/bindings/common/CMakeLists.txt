set(CCCR_BINDINGS_COMMON_HEADERS
    include/cccr/bindings/common/error.hpp
    include/cccr/bindings/common/pipeline.hpp
)

set(CCCR_BINDINGS_COMMON_SOURCES
    src/pipeline.cpp
)

add_library(cccr_bindings_common STATIC
    ${CCCR_BINDINGS_COMMON_HEADERS}
    ${CCCR_BINDINGS_COMMON_SOURCES}
)
add_library(cccr::bindings::common ALIAS cccr_bindings_common)

set_target_properties(cccr_bindings_common PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
    FOLDER bindings/common
)

target_include_directories(cccr_bindings_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(cccr_bindings_common PUBLIC
    cccr::core
)

cccr_saxon_che_apply_build_rpath(cccr_bindings_common)

if(CCCR_IPO_SUPPORTED)
  set_target_properties(cccr_bindings_common PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
      INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
  )
endif()
