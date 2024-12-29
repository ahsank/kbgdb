# src/http/CMakeLists.txt
add_library(kbgdb_http
    server.cpp
    handler.cpp
)

target_link_libraries(kbgdb_http
    PUBLIC
        kbgdb_includes
        kbgdb_common
        kbgdb_core
        kbgdb_query
        folly::folly
        Proxygen::proxygen
        Proxygen::httpserver
)
