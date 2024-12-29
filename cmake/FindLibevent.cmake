# cmake/FindLibevent.cmake
find_path(LIBEVENT_INCLUDE_DIR
    NAMES event.h
    PATHS
        /opt/homebrew/include
        /usr/local/include
)

find_library(LIBEVENT_LIBRARY
    NAMES event libevent
    PATHS
        /opt/homebrew/lib
        /usr/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libevent
    REQUIRED_VARS
        LIBEVENT_LIBRARY
        LIBEVENT_INCLUDE_DIR
)

if(Libevent_FOUND AND NOT TARGET Libevent::Libevent)
    add_library(Libevent::Libevent UNKNOWN IMPORTED)
    set_target_properties(Libevent::Libevent PROPERTIES
        IMPORTED_LOCATION "${LIBEVENT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBEVENT_INCLUDE_DIR}"
    )
endif()
