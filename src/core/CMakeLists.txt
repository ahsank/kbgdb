add_library(kbgdb_core
    rule.cpp
    knowledge_base.cpp
)

target_link_libraries(kbgdb_core
    PUBLIC
        kbgdb_includes
        kbgdb_common
        kbgdb_storage  # Add dependency on storage component
        folly::folly
)
