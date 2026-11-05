# set(SAXON_VERSION CACHE STRING) Main Saxon J version; must be set on command line with -D

set(saxonc "saxonc-${SaxonC_EDITION}")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(${saxonc} SHARED
    ${SAXONC_VERSION_SRC_DIR}/saxonc_version.c
    ${SAXONC_SRC_DIR}/lib/CPPUserFunctionHandler.cpp
    ${SAXONC_SRC_DIR}/lib/DocumentBuilder.cpp
    ${SAXONC_SRC_DIR}/lib/SaxonApiException.cpp
    ${SAXONC_SRC_DIR}/lib/SaxonCGlue.c
    ${SAXONC_SRC_DIR}/lib/SaxonCProcessor.c
    ${SAXONC_SRC_DIR}/lib/SaxonCXPath.c
    ${SAXONC_SRC_DIR}/lib/SaxonProcessor.cpp
    ${SAXONC_SRC_DIR}/lib/SchemaValidator.cpp
    ${SAXONC_SRC_DIR}/lib/XPathProcessor.cpp
    ${SAXONC_SRC_DIR}/lib/XQueryProcessor.cpp
    ${SAXONC_SRC_DIR}/lib/XdmArray.cpp
    ${SAXONC_SRC_DIR}/lib/XdmAtomicValue.cpp
    ${SAXONC_SRC_DIR}/lib/XdmFunctionItem.cpp
    ${SAXONC_SRC_DIR}/lib/XdmItem.cpp
    ${SAXONC_SRC_DIR}/lib/XdmMap.cpp
    ${SAXONC_SRC_DIR}/lib/XdmNode.cpp
    ${SAXONC_SRC_DIR}/lib/XdmValue.cpp
    ${SAXONC_SRC_DIR}/lib/Xslt30Processor.cpp
    ${SAXONC_SRC_DIR}/lib/XsltExecutable.cpp
    ${SAXONC_SRC_DIR}/lib/XsdCompiler.cpp
    ${SAXONC_SRC_DIR}/lib/XsdSchema.cpp
)
add_library(SaxonC${SAXON_EDITION}::${saxonc} ALIAS ${saxonc})
set_target_properties(${saxonc} PROPERTIES
    SOVERSION ${SaxonC_VERSION_MAJOR}
    VERSION ${SaxonC_VERSION}
)
target_link_libraries(${saxonc} PRIVATE Threads::Threads)

target_sources(${saxonc}
    PUBLIC
        FILE_SET common_headers
        TYPE HEADERS
        BASE_DIRS
            ${SAXONC_SRC_DIR}/include
        FILES
            ${SAXONC_SRC_DIR}/include/saxonc/saxonc_version.h
            ${SAXONC_SRC_DIR}/include/saxonc/DocumentBuilder.h
            ${SAXONC_SRC_DIR}/include/saxonc/SaxonApiException.h
            ${SAXONC_SRC_DIR}/include/saxonc/SaxonCGlue.h
            ${SAXONC_SRC_DIR}/include/saxonc/SaxonCProcessor.h
            ${SAXONC_SRC_DIR}/include/saxonc/SaxonCXPath.h
            ${SAXONC_SRC_DIR}/include/saxonc/SaxonProcessor.h
            ${SAXONC_SRC_DIR}/include/saxonc/SchemaValidator.h
            ${SAXONC_SRC_DIR}/include/saxonc/XsdSchema.h
            ${SAXONC_SRC_DIR}/include/saxonc/XsdCompiler.h
            ${SAXONC_SRC_DIR}/include/saxonc/UserFunctionHandler.h
            ${SAXONC_SRC_DIR}/include/saxonc/XPathProcessor.h
            ${SAXONC_SRC_DIR}/include/saxonc/XQueryProcessor.h
            ${SAXONC_SRC_DIR}/include/saxonc/XdmArray.h
            ${SAXONC_SRC_DIR}/include/saxonc/XdmAtomicValue.h
            ${SAXONC_SRC_DIR}/include/saxonc/XdmFunctionItem.h
            ${SAXONC_SRC_DIR}/include/saxonc/XdmItem.h
            ${SAXONC_SRC_DIR}/include/saxonc/XdmMap.h
            ${SAXONC_SRC_DIR}/include/saxonc/XdmNode.h
            ${SAXONC_SRC_DIR}/include/saxonc/XdmValue.h
            ${SAXONC_SRC_DIR}/include/saxonc/Xslt30Processor.h
            ${SAXONC_SRC_DIR}/include/saxonc/XsltExecutable.h
)
target_sources(${saxonc}
    PUBLIC
        FILE_SET edition_headers
        TYPE HEADERS
        BASE_DIRS
            ${SAXONC_EDITION_SRC_DIR}/include
        FILES
            ${SAXONC_EDITION_SRC_DIR}/include/saxonc/saxonc-core.h
)

if(NOT(DEFINED NO_BUNDLED_SAXONC_CORE AND NO_BUNDLED_SAXONC_CORE))
    list(APPEND CMAKE_PREFIX_PATH ${BUNDLED_SAXONC_CORE_DIR})
endif()
find_package(SaxonCCore${SAXON_EDITION} ${SaxonC_VERSION} EXACT REQUIRED)

target_link_libraries(${saxonc} PUBLIC SaxonCCore${SAXON_EDITION}::saxonc-core-${SaxonC_EDITION})
