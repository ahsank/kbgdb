# Simple query example (no external dependencies)
add_executable(simple_query
    simple_query.cpp
)

target_link_libraries(simple_query
    PRIVATE
        kbgdb_core
        kbgdb_query
)

# Interactive REPL
add_executable(repl
    repl.cpp
)

target_link_libraries(repl
    PRIVATE
        kbgdb_core
        kbgdb_query
)
