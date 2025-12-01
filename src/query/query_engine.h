#pragma once
#include "core/knowledge_base.h"
#include <memory>
#include <string>

namespace kbgdb {

/**
 * QueryResult encapsulates the result of a query execution.
 */
struct QueryResult {
    bool success = false;
    std::vector<BindingSet> bindings;
    std::string error;
    
    std::string toJSON() const;
};

/**
 * QueryEngine provides a simple interface for executing queries.
 * This is a synchronous implementation.
 */
class QueryEngine {
public:
    explicit QueryEngine(std::shared_ptr<KnowledgeBase> kb);
    
    /**
     * Execute a query and return results.
     * Query format: "predicate(?Var1, constant, ?Var2)"
     */
    QueryResult execute(const std::string& queryStr);
    
private:
    std::shared_ptr<KnowledgeBase> kb_;
};

} // namespace kbgdb
