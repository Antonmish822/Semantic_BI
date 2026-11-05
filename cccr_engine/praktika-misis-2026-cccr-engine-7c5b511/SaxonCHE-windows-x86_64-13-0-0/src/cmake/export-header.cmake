target_sources(${saxonc}
    PUBLIC
        FILE_SET export_header
        TYPE HEADERS
        BASE_DIRS
            ${SAXONC_SRC_DIR}
        FILES
            ${SAXONC_SRC_DIR}/include/saxonc_export.h
)
