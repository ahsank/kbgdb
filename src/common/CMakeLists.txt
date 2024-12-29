add_library(kbgdb_common
    fact.cpp
)

target_link_libraries(kbgdb_common
    PUBLIC
        kbgdb_includes
        folly::folly
)
