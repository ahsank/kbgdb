# cmake/FindSodium.cmake
include(FindPackageHandleStandardArgs)

find_path(SODIUM_INCLUDE_DIR
    NAMES sodium.h
    PATHS
        ${SODIUM_ROOT}/include
        /opt/homebrew/include
        /usr/local/include
        /usr/include
)

find_library(SODIUM_LIBRARY
    NAMES sodium libsodium
    PATHS
        ${SODIUM_ROOT}/lib
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

find_package_handle_standard_args(Sodium
    REQUIRED_VARS
        SODIUM_LIBRARY
        SODIUM_INCLUDE_DIR
)

if(Sodium_FOUND AND NOT TARGET Sodium::Sodium)
    add_library(Sodium::Sodium UNKNOWN IMPORTED)
    set_target_properties(Sodium::Sodium PROPERTIES
        IMPORTED_LOCATION "${SODIUM_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(SODIUM_INCLUDE_DIR SODIUM_LIBRARY)
