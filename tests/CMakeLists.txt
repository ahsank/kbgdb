# Find GTest
find_package(GTest REQUIRED)
include(GoogleTest)

# Common tests
add_executable(common_tests
    common/fact_test.cpp
)

target_link_libraries(common_tests
    PRIVATE
        kbgdb_common
        GTest::GTest
        GTest::Main
        GTest::gmock_main
)

# Query parser tests
add_executable(query_tests
    query/query_parser_test.cpp
)

target_link_libraries(query_tests
    PRIVATE
        kbgdb_query
        GTest::GTest
        GTest::Main
        GTest::gmock_main
)

# Core tests (knowledge base, rules, etc.)
add_executable(core_tests
    core/knowledge_base_test.cpp
    core/rule_test.cpp
)

target_link_libraries(core_tests
    PRIVATE
        kbgdb_core
        GTest::GTest
        GTest::Main
        GTest::gmock_main
)

# Register tests
gtest_discover_tests(common_tests)
gtest_discover_tests(query_tests)
gtest_discover_tests(core_tests)
