# Add components in dependency order
add_subdirectory(common)   # First because others depend on it
add_subdirectory(storage)  # Depends on common
add_subdirectory(core)     # Depends on common and storage
add_subdirectory(query)    # Depends on core
add_subdirectory(http)     # Depends on core and query
