# examples/CMakeLists.txt
add_executable(kbgdb_service_example
    service.cpp
)

target_link_libraries(kbgdb_service_example
    PRIVATE
        kbgdb_includes
        kbgdb_http
        folly::folly
        gflags::gflags
)

# Optional: Add another example
add_executable(simple_query_example
    simple_query.cpp
)

target_link_libraries(simple_query_example
    PRIVATE
        kbgdb_includes
        kbgdb_core
        kbgdb_storage
        kbgdb_query
        kbgdb_http
        folly::folly
)
