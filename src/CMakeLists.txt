# Add components in dependency order
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(query)

if(BUILD_ASYNC)
    add_subdirectory(async)
    add_subdirectory(storage)
    add_subdirectory(http)
endif()
