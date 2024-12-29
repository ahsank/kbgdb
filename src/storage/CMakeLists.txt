add_library(kbgdb_storage
    rocksdb_provider.cpp
)

target_link_libraries(kbgdb_storage
    PUBLIC
        kbgdb_includes
        kbgdb_common
        folly::folly
        RocksDB::rocksdb
)