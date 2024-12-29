# For macOS, add Homebrew paths
if(APPLE)
    # Set Boost specific variables
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
endif()


# Set default dependency source
if(USE_SYSTEM_DEPS)
    # Find all required packages
    find_package(Boost REQUIRED COMPONENTS 
        context 
        filesystem 
        program_options 
        regex 
        system 
        thread
    )

    # Verify Boost was found
    if(NOT Boost_FOUND)
        message(FATAL_ERROR "Could not find Boost")
    endif()

    find_package(fmt  REQUIRED)
    find_package(Libevent REQUIRED)
    find_package(gflags REQUIRED)
    # find_package(glog REQUIRED)
    find_package(folly REQUIRED)
    find_package(RocksDB REQUIRED)
else()
    # Fetch and build dependencies
    
    # Folly requires several dependencies
    find_package(Boost REQUIRED COMPONENTS 
        context filesystem program_options regex system thread
    )
    find_package(OpenSSL REQUIRED)
    find_package(Gflags REQUIRED)
    find_package(ZLIB REQUIRED)
    find_package(BZip2 REQUIRED)
    find_package(LibLZMA REQUIRED)
    # find_package(LZ4 REQUIRED)
    find_package(ZSTD REQUIRED)
    find_package(Snappy REQUIRED)
    # find_package(LibDwarf REQUIRED)
    # find_package(LibUnwind REQUIRED)
    # find_package(DoubleConversion REQUIRED)
    # find_package(Glog REQUIRED)
    find_package(Fmt REQUIRED)

    # Fetch Folly
    FetchContent_Declare(
        folly
        GIT_REPOSITORY https://github.com/facebook/folly.git
        GIT_TAG v2024.01.15.00
    )
    FetchContent_MakeAvailable(folly)

    # Fetch RocksDB
    FetchContent_Declare(
        rocksdb
        GIT_REPOSITORY https://github.com/facebook/rocksdb.git
        GIT_TAG v8.1.1
    )
    FetchContent_MakeAvailable(rocksdb)

    # Fetch Proxygen
    FetchContent_Declare(
        proxygen
        GIT_REPOSITORY https://github.com/facebook/proxygen.git
        GIT_TAG v2024.01.15.00
    )
    FetchContent_MakeAvailable(proxygen)
endif()
