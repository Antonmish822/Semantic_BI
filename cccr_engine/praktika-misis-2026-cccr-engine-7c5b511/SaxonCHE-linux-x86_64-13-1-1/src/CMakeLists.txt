cmake_minimum_required(VERSION 3.27)
project(SaxonCHE VERSION 13.1.1)

set(SaxonC_VERSION ${SaxonCHE_VERSION})
set(SaxonC_VERSION_MAJOR ${SaxonCHE_VERSION_MAJOR})
set(SaxonC_EDITION "he" CACHE STRING "Saxon edition")
set(SAXON_EDITION "HE")
set(SAXONC_SRC_DIR .)
set(SAXONC_VERSION_SRC_DIR lib)
set(SAXONC_EDITION_SRC_DIR .)
set(BUNDLED_SAXONC_CORE_DIR ../SaxonCCoreHE)

include("cmake/libsaxonc.cmake")
include("cmake/export-header.cmake")
