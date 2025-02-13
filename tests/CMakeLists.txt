# tests/CMakeLists.txt

# Find GTest and GMock
find_package(GTest REQUIRED)
include(GoogleTest)

# Add test executables
add_executable(common_tests
    common/fact_test.cpp
)

add_executable(core_tests
    core/knowledge_base_test.cpp
    core/rule_test.cpp
)

add_executable(query_tests
    query/query_parser_test.cpp
#     query/query_engine_test.cpp
)

# Link test executables with their dependencies
target_link_libraries(common_tests
    PRIVATE
    kbgdb_common
    GTest::GTest
    GTest::Main
    GTest::gmock_main
)

target_link_libraries(core_tests
    PRIVATE
    kbgdb_core
    kbgdb_common
    kbgdb_storage
    kbgdb_query
    folly::folly
    GTest::GTest
    GTest::Main
    GTest::gmock_main
)

target_link_libraries(query_tests
    PRIVATE
    kbgdb_common
    kbgdb_query
    GTest::GTest
    GTest::Main
    GTest::gmock_main
)

# Register tests with CTest
gtest_discover_tests(common_tests)
gtest_discover_tests(core_tests)
gtest_discover_tests(query_tests)