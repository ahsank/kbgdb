# cmake/Findfolly.cmake
include(FindPackageHandleStandardArgs)

find_path(folly_INCLUDE_DIR
    NAMES folly/String.h
    PATHS
        ${FOLLY_ROOT}/include
        /opt/homebrew/include
        /usr/local/include
        /usr/include
)

find_library(folly_LIBRARY
    NAMES folly
    PATHS
        ${FOLLY_ROOT}/lib
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

# Find dependencies that Folly requires
find_package(Boost REQUIRED COMPONENTS context filesystem program_options regex system thread)
find_package(double-conversion REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

find_package_handle_standard_args(folly
    REQUIRED_VARS
        folly_LIBRARY
        folly_INCLUDE_DIR
)

if(folly_FOUND AND NOT TARGET folly::folly)
    add_library(folly::folly UNKNOWN IMPORTED)
    set_target_properties(folly::folly PROPERTIES
        IMPORTED_LOCATION "${folly_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${folly_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "Boost::context;Boost::filesystem;Boost::program_options;Boost::regex;Boost::system;Boost::thread;double-conversion::double-conversion;gflags::gflags;glog::glog;OpenSSL::SSL;OpenSSL::Crypto;Threads::Threads;fmt::fmt;;Libevent::Libevent"
    )
endif()

mark_as_advanced(folly_INCLUDE_DIR folly_LIBRARY)
