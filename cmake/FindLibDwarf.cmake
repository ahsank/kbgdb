# cmake/FindLibDwarf.cmake
include(FindPackageHandleStandardArgs)

find_path(LIBDWARF_INCLUDE_DIR
    NAMES libdwarf.h dwarf.h
    PATHS
        ${LIBDWARF_ROOT}/include
        /opt/homebrew/include
        /opt/homebrew/include/libdwarf-0
        /usr/local/include
        /usr/include
)

find_library(LIBDWARF_LIBRARY
    NAMES dwarf libdwarf
    PATHS
        ${LIBDWARF_ROOT}/lib
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

find_package_handle_standard_args(LibDwarf
    REQUIRED_VARS
        LIBDWARF_LIBRARY
        LIBDWARF_INCLUDE_DIR
)

if(LibDwarf_FOUND AND NOT TARGET LibDwarf::LibDwarf)
    add_library(LibDwarf::LibDwarf UNKNOWN IMPORTED)
    set_target_properties(LibDwarf::LibDwarf PROPERTIES
        IMPORTED_LOCATION "${LIBDWARF_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBDWARF_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(LIBDWARF_INCLUDE_DIR LIBDWARF_LIBRARY)
