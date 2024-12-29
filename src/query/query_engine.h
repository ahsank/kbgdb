#pragma once
#include "core/knowledge_base.h"
#include <folly/futures/Future.h>
#include <memory>

namespace kbgdb {

struct QueryResult {
    bool success;
    std::vector<BindingSet> bindings;
    std::string error;
    
    std::string toJSON() const;
};

class QueryEngine {
public:
    explicit QueryEngine(std::shared_ptr<KnowledgeBase> kb);
    
    folly::Future<QueryResult> executeQuery(const std::string& query_str);

private:
    std::shared_ptr<KnowledgeBase> kb_;
    
    folly::Future<std::vector<BindingSet>> proveGoal(
        const Fact& goal,
        const BindingSet& bindings
    );
    
    std::optional<BindingSet> unify(
        const Fact& goal,
        const Fact& fact,
        const BindingSet& bindings
    );
    
    static std::vector<BindingSet> combineBindings(
        const std::vector<std::vector<BindingSet>>& allBindings
    );
};

} // namespace kbgdb
