include(FindPackageHandleStandardArgs)

find_path(PROXYGEN_INCLUDE_DIR
    NAMES proxygen/httpserver/HTTPServer.h
    PATHS
        ${PROXYGEN_ROOT}/include
        /opt/homebrew/include
        /usr/local/include
        /usr/include
)

find_library(PROXYGEN_LIBRARY
    NAMES proxygen
    PATHS
        ${PROXYGEN_ROOT}/lib
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

find_library(PROXYGEN_HTTP_SERVER_LIBRARY
    NAMES proxygenhttpserver
    PATHS
        ${PROXYGEN_ROOT}/lib
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

find_package(folly REQUIRED)

find_package_handle_standard_args(Proxygen
    REQUIRED_VARS
        PROXYGEN_LIBRARY
        PROXYGEN_HTTP_SERVER_LIBRARY
        PROXYGEN_INCLUDE_DIR
)

if(Proxygen_FOUND AND NOT TARGET Proxygen::proxygen)
    add_library(Proxygen::proxygen UNKNOWN IMPORTED)
    set_target_properties(Proxygen::proxygen PROPERTIES
        IMPORTED_LOCATION "${PROXYGEN_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PROXYGEN_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "folly::folly"
    )
    
    add_library(Proxygen::httpserver UNKNOWN IMPORTED)
    set_target_properties(Proxygen::httpserver PROPERTIES
        IMPORTED_LOCATION "${PROXYGEN_HTTP_SERVER_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PROXYGEN_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "Proxygen::proxygen"
    )
endif()

mark_as_advanced(
    PROXYGEN_INCLUDE_DIR
    PROXYGEN_LIBRARY
    PROXYGEN_HTTP_SERVER_LIBRARY
)