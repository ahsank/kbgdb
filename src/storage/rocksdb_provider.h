#pragma once
#include "storage/storage_provider.h"
#include <rocksdb/db.h>
#include <memory>

namespace kbgdb {

class RocksDBProvider : public StorageProvider {
public:
    explicit RocksDBProvider(const std::string& db_path);
    
    bool canHandle(const Fact& pattern) override;
    folly::Future<std::vector<Fact>> getFacts(const Fact& pattern) override;

private:
    std::unique_ptr<rocksdb::DB> db_;
};

} // namespace kbgdb
