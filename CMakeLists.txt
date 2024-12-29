cmake_minimum_required(VERSION 3.16)
project(kbgdb VERSION 1.0.0 LANGUAGES CXX)

# Add policy settings
# if(POLICY CMP0144)
#     cmake_policy(SET CMP0144 NEW)
# endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


# Options
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(USE_SYSTEM_DEPS "Use system-installed dependencies" ON)

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()
# First add includes library
add_library(kbgdb_includes INTERFACE)
target_include_directories(kbgdb_includes
    BEFORE
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Include dependencies handling
include(Dependencies)

# Add components
add_subdirectory(src)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()