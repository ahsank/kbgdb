add_library(kbgdb_query
    query_parser.cpp
    query_engine.cpp
)

target_link_libraries(kbgdb_query
    PUBLIC
        kbgdb_includes
        kbgdb_common
)
