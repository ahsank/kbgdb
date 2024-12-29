#pragma once
#include "common/fact.h"
#include <folly/futures/Future.h>
#include <vector>

namespace kbgdb {

class StorageProvider {
public:
    virtual ~StorageProvider() = default;
    
    virtual bool canHandle(const Fact& pattern) = 0;
    virtual folly::Future<std::vector<Fact>> getFacts(const Fact& pattern) = 0;
};

} // namespace kbgdb
