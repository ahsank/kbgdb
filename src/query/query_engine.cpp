// src/query/query_engine.cpp
#include "query/query_engine.h"
#include "query/query_parser.h"
#include <folly/executors/GlobalExecutor.h>
#include <folly/futures/Future.h>
#include <folly/json.h>

namespace kbgdb {

std::string QueryResult::toJSON() const {
    folly::dynamic json = folly::dynamic::object;
    json["success"] = success;
    
    if (!success) {
        json["error"] = error;
        return folly::toJson(json);
    }
    
    folly::dynamic bindingsJson = folly::dynamic::array;
    for (const auto& binding : bindings) {
        folly::dynamic bindingJson = folly::dynamic::object;
        for (const auto& [var, value] : binding.bindings) {
            bindingJson[var] = value;
        }
        bindingsJson.push_back(std::move(bindingJson));
    }
    
    json["bindings"] = std::move(bindingsJson);
    return folly::toJson(json);
}

QueryEngine::QueryEngine(std::shared_ptr<KnowledgeBase> kb)
    : kb_(std::move(kb)) {
}

folly::Future<QueryResult> QueryEngine::executeQuery(const std::string& query_str) {
    return folly::via(folly::getGlobalCPUExecutor().get(), [this, query_str]() {
        try {
            QueryParser parser;
            Fact queryFact = parser.parse(query_str);
            
            return proveGoal(queryFact, BindingSet{})
                .via(folly::getGlobalCPUExecutor().get())
                .thenValue([](std::vector<BindingSet> bindings) {
                    return QueryResult{
                        .success = true,
                        .bindings = std::move(bindings),
                        .error = ""
                    };
                })
                .thenError<std::exception>([](const std::exception& e) {
                    return QueryResult{
                        .success = false,
                        .bindings = {},
                        .error = e.what()
                    };
                });
        } catch (const std::exception& e) {
            return folly::makeFuture<QueryResult>(QueryResult{
                .success = false,
                .bindings = {},
                .error = e.what()
            });
        }
    });
}

folly::Future<std::vector<BindingSet>> 
QueryEngine::proveGoal(const Fact& goal, const BindingSet& bindings) {
    std::vector<folly::Future<std::vector<BindingSet>>> futures;

    // Check memory facts
    auto memoryFacts = kb_->getMemoryFacts(goal.getPredicate());
    for (const auto& fact : memoryFacts) {
        if (auto newBindings = unify(goal, fact, bindings)) {
            futures.push_back(folly::makeFuture<std::vector<BindingSet>>({*newBindings}));
        }
    }

    // Check external providers
    for (const auto& provider : kb_->getExternalProviders()) {
        if (provider->canHandle(goal)) {
            futures.push_back(
                provider->getFacts(goal)
                    .via(folly::getGlobalCPUExecutor().get())
                    .thenValue([this, goal, bindings](std::vector<Fact> facts) {
                        std::vector<BindingSet> results;
                        for (const auto& fact : facts) {
                            if (auto newBindings = unify(goal, fact, bindings)) {
                                results.push_back(*newBindings);
                            }
                        }
                        return results;
                    })
            );
        }
    }

    // Check rules
    for (const auto& rule : kb_->getRules()) {
        if (rule.getHead().getPredicate() == goal.getPredicate()) {
            if (auto headBindings = unify(goal, rule.getHead(), bindings)) {
                // Prove all body goals
                std::vector<folly::Future<std::vector<BindingSet>>> bodyFutures;
                for (const auto& bodyGoal : rule.getBody()) {
                    bodyFutures.push_back(proveGoal(bodyGoal, *headBindings));
                }

                futures.push_back(
                    folly::collect(bodyFutures)
                        .via(folly::getGlobalCPUExecutor().get())
                        .thenValue([](std::vector<std::vector<BindingSet>> bodyResults) {
                            return combineBindings(bodyResults);
                        })
                );
            }
        }
    }

    return folly::collect(futures)
        .via(folly::getGlobalCPUExecutor().get())
        .thenValue([](std::vector<std::vector<BindingSet>> allResults) {
            std::vector<BindingSet> combined;
            for (const auto& results : allResults) {
                combined.insert(combined.end(), results.begin(), results.end());
            }
            return combined;
        });
}

std::optional<BindingSet> 
QueryEngine::unify(const Fact& goal, const Fact& fact, const BindingSet& bindings) {
    if (goal.getPredicate() != fact.getPredicate() ||
        goal.getTerms().size() != fact.getTerms().size()) {
        return std::nullopt;
    }

    BindingSet newBindings = bindings;
    const auto& goalTerms = goal.getTerms();
    const auto& factTerms = fact.getTerms();

    for (size_t i = 0; i < goalTerms.size(); ++i) {
        const auto& goalTerm = goalTerms[i];
        const auto& factTerm = factTerms[i];

        if (goalTerm.isVariable()) {
            const std::string& var = goalTerm.value;
            const std::string& existing = newBindings.get(var);

            if (existing.empty()) {
                newBindings.add(var, factTerm.value);
            } else if (existing != factTerm.value) {
                return std::nullopt;
            }
        } else if (goalTerm.value != factTerm.value) {
            return std::nullopt;
        }
    }

    return newBindings;
}

std::vector<BindingSet> 
QueryEngine::combineBindings(const std::vector<std::vector<BindingSet>>& allBindings) {
    if (allBindings.empty()) {
        return {};
    }

    std::vector<BindingSet> result = allBindings[0];

    for (size_t i = 1; i < allBindings.size(); ++i) {
        std::vector<BindingSet> newResult;
        for (const auto& existing : result) {
            for (const auto& current : allBindings[i]) {
                bool compatible = true;
                BindingSet combined = existing;

                for (const auto& [var, value] : current.bindings) {
                    const std::string& existingValue = existing.get(var);
                    if (!existingValue.empty() && existingValue != value) {
                        compatible = false;
                        break;
                    }
                    combined.add(var, value);
                }

                if (compatible) {
                    newResult.push_back(std::move(combined));
                }
            }
        }
        result = std::move(newResult);
    }

    return result;
}

} // namespace kbgdb
