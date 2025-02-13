#pragma once
#include "common/fact.h"
#include "core/rule.h"
#include "storage/storage_provider.h"
#include <folly/futures/Future.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace kbgdb {

struct BindingSet {
    std::unordered_map<std::string, std::string> bindings;
    
    void add(const std::string& var, const std::string& value) {
        bindings[var] = value;
    }
    
    std::string get(const std::string& var) const {
        auto it = bindings.find(var);
        return it != bindings.end() ? it->second : "";
    }
};

class KnowledgeBase {
public:
    explicit KnowledgeBase(const std::string& init_file);
    
    folly::Future<std::vector<BindingSet>> query(const std::string& query_str);
    
    void addFact(const Fact& fact);
    void addRule(const Rule& rule);
    void addExternalProvider(std::unique_ptr<StorageProvider> provider);
    
    const std::vector<Rule>& getRules() const { return rules_; }
    const std::vector<Fact>& getMemoryFacts(const std::string& predicate) const;
    const std::vector<std::unique_ptr<StorageProvider>>& getExternalProviders() const {
        return external_providers_;
    }
    folly::Future<std::vector<BindingSet>> evaluateGoal(
        const Fact& goal,
        const BindingSet& bindings) const ;

    std::optional<BindingSet> unifyFact(
        const Fact& goal,
        const Fact& fact,
        const BindingSet& currentBindings) const;

    void loadFromFile(const std::string& filename);

private:
    std::vector<Rule> rules_;
    std::unordered_map<std::string, std::vector<Fact>> memory_facts_;
    std::vector<std::unique_ptr<StorageProvider>> external_providers_;
};

} // namespace kbgdb
