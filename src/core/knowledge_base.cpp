#include  "knowledge_base.h"
#include <query/query_parser.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace kbgdb {
// Helper function to handle rule evaluation
// In knowledge_base.cpp, update RuleEvaluator class
class RuleEvaluator {
public:
    explicit RuleEvaluator(const KnowledgeBase* kb) : kb_(kb) {}

    folly::Future<std::vector<BindingSet>> evaluateRule(
        const Rule& rule,
        const Fact& query,
        BindingSet initialBindings = BindingSet{}) {
        
        std::cout << "Evaluating rule: " << rule.toString() << std::endl;
        std::cout << "For query: " << query.toString() << std::endl;
        std::cout << "Initial bindings: " << bindingsToString(initialBindings) << std::endl;
        
        // First unify query with rule head
        auto headBindings = unifyTerms(query.getTerms(), rule.getHead().getTerms(), initialBindings);
        if (!headBindings) {
            std::cout << "Head unification failed" << std::endl;
            return folly::makeFuture<std::vector<BindingSet>>(std::vector<BindingSet>{});
        }

        std::cout << "Head unified with bindings: " << bindingsToString(*headBindings) << std::endl;

        // Now evaluate all body goals with the bindings
        return evaluateBodyGoals(rule.getBody(), *headBindings);
    }

private:
    const KnowledgeBase* kb_;

    std::string bindingsToString(const BindingSet& bindings) const {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& [var, val] : bindings.bindings) {
            if (!first) oss << ", ";
            oss << var << "=" << val;
            first = false;
        }
        oss << "}";
        return oss.str();
    }

    std::optional<BindingSet> unifyTerms(
        const std::vector<Term>& queryTerms,
        const std::vector<Term>& ruleTerms,
        const BindingSet& currentBindings) {
        
        if (queryTerms.size() != ruleTerms.size()) {
            return std::nullopt;
        }

        BindingSet newBindings = currentBindings;
        for (size_t i = 0; i < queryTerms.size(); i++) {
            const auto& queryTerm = queryTerms[i];
            const auto& ruleTerm = ruleTerms[i];

            std::cout << "Unifying query term: " << queryTerm.toString() 
                      << " with rule term: " << ruleTerm.toString() << std::endl;

            if (queryTerm.isVariable()) {
                auto existingQueryBinding = newBindings.get(queryTerm.value);
                if (!existingQueryBinding.empty()) {
                    if (ruleTerm.isVariable()) {
                        newBindings.add(ruleTerm.value, existingQueryBinding);
                    } else if (existingQueryBinding != ruleTerm.value) {
                        std::cout << "Failed: Query variable bound to different value" << std::endl;
                        return std::nullopt;
                    }
                } else if (ruleTerm.isVariable()) {
                    auto existingRuleBinding = newBindings.get(ruleTerm.value);
                    if (!existingRuleBinding.empty()) {
                        newBindings.add(queryTerm.value, existingRuleBinding);
                    } else {
                        // Both are unbound variables, bind them together
                        newBindings.add(queryTerm.value, ruleTerm.value);
                    }
                } else {
                    newBindings.add(queryTerm.value, ruleTerm.value);
                }
            } else if (ruleTerm.isVariable()) {
                auto existingBinding = newBindings.get(ruleTerm.value);
                if (!existingBinding.empty() && existingBinding != queryTerm.value) {
                    std::cout << "Failed: Rule variable bound to different value" << std::endl;
                    return std::nullopt;
                }
                newBindings.add(ruleTerm.value, queryTerm.value);
            } else if (queryTerm.value != ruleTerm.value) {
                std::cout << "Failed: Constant terms don't match" << std::endl;
                return std::nullopt;
            }
        }

        std::cout << "Unified with bindings: " << bindingsToString(newBindings) << std::endl;
        return newBindings;
    }
folly::Future<std::vector<BindingSet>> evaluateBodyGoals(
    const std::vector<Fact>& goals,
    const BindingSet& bindings) {
    
    if (goals.empty()) {
        return folly::makeFuture<std::vector<BindingSet>>(
            std::vector<BindingSet>{bindings});
    }

    // Evaluate first goal
    std::vector<Fact> remainingGoals(goals.begin() + 1, goals.end());
    
    return kb_->evaluateGoal(goals[0], bindings)
        .via(folly::getGlobalCPUExecutor().get())
        .thenValue([this, remainingGoals]
                  (std::vector<BindingSet> firstGoalBindings) 
                  -> folly::Future<std::vector<BindingSet>> {
            if (firstGoalBindings.empty() || remainingGoals.empty()) {
                // Create a new vector instead of passing const reference
                return folly::makeFuture<std::vector<BindingSet>>(
                    std::vector<BindingSet>(firstGoalBindings));
            }

            // For each binding from first goal, evaluate remaining goals
            std::vector<folly::Future<std::vector<BindingSet>>> futures;
            for (const auto& binding : firstGoalBindings) {
                futures.push_back(evaluateBodyGoals(remainingGoals, binding));
            }

            return folly::collect(futures)
                .via(folly::getGlobalCPUExecutor().get())
                .thenValue([](std::vector<std::vector<BindingSet>> results) 
                          -> std::vector<BindingSet> {
                    std::vector<BindingSet> flattened;
                    for (const auto& result : results) {
                        flattened.insert(flattened.end(), result.begin(), result.end());
                    }
                    return flattened;
                });
        });
}

};

KnowledgeBase::KnowledgeBase(const std::string& init_file) {
    if (!init_file.empty()) {
        loadFromFile(init_file);
    }
}

void KnowledgeBase::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    QueryParser parser;
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '%') continue;
        
        try {
            if (line.find(":-") != std::string::npos) {
                parser.setRuleMode(true);
                
                size_t pos = line.find(":-");
                std::string head = line.substr(0, pos);
                std::string body = line.substr(pos + 2);
                
                // Trim whitespace from head and body
                head.erase(0, head.find_first_not_of(" \t"));
                head.erase(head.find_last_not_of(" \t.") + 1);
                body.erase(0, body.find_first_not_of(" \t"));
                body.erase(body.find_last_not_of(" \t.") + 1);
                
                std::cout << "Parsing rule head: " << head << std::endl;
                Fact headFact = parser.parse(head);
                
                // Split body into separate goals, respecting parentheses
                std::vector<Fact> bodyGoals;
                std::string currentGoal;
                int parenCount = 0;
                
                for (size_t i = 0; i < body.length(); ++i) {
                    char c = body[i];
                    if (c == '(') {
                        parenCount++;
                        currentGoal += c;
                    } else if (c == ')') {
                        parenCount--;
                        currentGoal += c;
                        
                        // If we've closed all parentheses and there's a comma next, 
                        // or we're at the end, this is a complete goal
                        if (parenCount == 0 && 
                            (i + 1 >= body.length() || body[i + 1] == ',')) {
                            // Trim and parse goal
                            currentGoal.erase(0, currentGoal.find_first_not_of(" \t"));
                            currentGoal.erase(currentGoal.find_last_not_of(" \t.") + 1);
                            
                            if (!currentGoal.empty()) {
                                std::cout << "Parsing complete body goal: '" 
                                         << currentGoal << "'" << std::endl;
                                bodyGoals.push_back(parser.parse(currentGoal));
                            }
                            currentGoal.clear();
                            
                            // Skip the comma
                            if (i + 1 < body.length() && body[i + 1] == ',') {
                                i++;
                            }
                        }
                    } else if (parenCount > 0 || !std::isspace(c)) {
                        // Only add non-space characters if we're inside parentheses
                        // or it's not a space
                        currentGoal += c;
                    }
                }
                
                std::cout << "Creating rule with head: " << headFact.toString() 
                         << " and " << bodyGoals.size() << " body goals" << std::endl;
                
                rules_.push_back(Rule(headFact, bodyGoals));
                std::cout << "Added rule. Total rules now: " << rules_.size() << std::endl;
                
            } else {
                parser.setRuleMode(false);
                addFact(parser.parse(line));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line: " << line << std::endl;
            std::cerr << "Error details: " << e.what() << std::endl;
            throw e;
        }
    }
    
    // Print loaded facts and rules
    std::cout << "\nLoaded facts:" << std::endl;
    for (const auto& [pred, facts] : memory_facts_) {
        std::cout << "  " << pred << ":" << std::endl;
        for (const auto& fact : facts) {
            std::cout << "    " << fact.toString() << std::endl;
        }
    }
    
    std::cout << "\nLoaded rules:" << std::endl;
    for (const auto& rule : rules_) {
        std::cout << "  " << rule.toString() << std::endl;
    }
}

void KnowledgeBase::addFact(const Fact& fact) {
    if (fact.getPredicate().empty()) {
        std::cerr << "Warning: Attempting to add fact with empty predicate" << std::endl;
        return;
    }
    
    std::cout << "Adding fact: " << fact.toString() << std::endl;
    memory_facts_[fact.getPredicate()].push_back(fact);
}

void KnowledgeBase::addRule(const Rule& rule) {
    if (!rule.isValid()) {
        std::cerr << "Warning: Attempting to add invalid rule" << std::endl;
        return;
    }
    
    std::cout << "Adding rule: " << rule.toString() << std::endl;
    rules_.push_back(rule);
}

void KnowledgeBase::addExternalProvider(std::unique_ptr<StorageProvider> provider) {
    external_providers_.push_back(std::move(provider));
}

const std::vector<Fact>& KnowledgeBase::getMemoryFacts(const std::string& predicate) const {
    static const std::vector<Fact> empty_vector;
    
    auto it = memory_facts_.find(predicate);
    if (it == memory_facts_.end()) {
        std::cout << "Warning: No facts found for predicate: " << predicate << std::endl;
        return empty_vector;
    }
    
    return it->second;
}


// Add unifyFact helper method to KnowledgeBase
std::optional<BindingSet> KnowledgeBase::unifyFact(
    const Fact& goal,
    const Fact& fact,
    const BindingSet& currentBindings) const {
    
    if (goal.getPredicate() != fact.getPredicate() ||
        goal.getTerms().size() != fact.getTerms().size()) {
        return std::nullopt;
    }

    BindingSet newBindings = currentBindings;
    const auto& goalTerms = goal.getTerms();
    const auto& factTerms = fact.getTerms();

    for (size_t i = 0; i < goalTerms.size(); i++) {
        const auto& goalTerm = goalTerms[i];
        const auto& factTerm = factTerms[i];

        if (goalTerm.isVariable()) {
            auto existingValue = newBindings.get(goalTerm.value);
            if (!existingValue.empty()) {
                if (existingValue != factTerm.value) {
                    return std::nullopt;
                }
            } else {
                newBindings.add(goalTerm.value, factTerm.value);
            }
        } else if (goalTerm.value != factTerm.value) {
            return std::nullopt;
        }
    }

    return newBindings;
}

folly::Future<std::vector<BindingSet>> KnowledgeBase::evaluateGoal(
    const Fact& goal,
    const BindingSet& bindings) const {
    
    std::cout << "evaluateGoal called with: " << goal.toString() << std::endl;
    
    // First try direct facts
    std::vector<BindingSet> factResults;
    auto facts = getMemoryFacts(goal.getPredicate());
    
    std::cout << "Found " << facts.size() << " facts for predicate: " 
              << goal.getPredicate() << std::endl;

    for (const auto& fact : facts) {
        std::cout << "Checking fact: " << fact.toString() << std::endl;
        if (auto newBindings = unifyFact(goal, fact, bindings)) {
            factResults.push_back(*newBindings);
        }
    }

    // Then try rules
    std::vector<folly::Future<std::vector<BindingSet>>> ruleResults;
    RuleEvaluator evaluator(this);
    
    const auto& allRules = rules_;  // Get reference to rules
    std::cout << "Checking " << allRules.size() << " rules" << std::endl;
    
    for (const auto& rule : allRules) {
        std::cout << "Evaluating rule: " << rule.toString() << std::endl;
        if (rule.getHead().getPredicate() == goal.getPredicate()) {
            std::cout << "Rule predicate matches, evaluating..." << std::endl;
            ruleResults.push_back(evaluator.evaluateRule(rule, goal, bindings));
        }
    }

    return folly::collect(ruleResults)
        .via(folly::getGlobalCPUExecutor().get())
        .thenValue([factResults = std::move(factResults)]
                  (std::vector<std::vector<BindingSet>> ruleBindings) {
            std::vector<BindingSet> allResults = factResults;
            for (const auto& ruleResult : ruleBindings) {
                allResults.insert(allResults.end(), 
                                ruleResult.begin(), 
                                ruleResult.end());
            }
            return allResults;
        });
}


// Update the query method
folly::Future<std::vector<BindingSet>> KnowledgeBase::query(const std::string& query_str) {
    try {
        QueryParser parser;
        Fact queryFact = parser.parse(query_str);
        return evaluateGoal(queryFact, BindingSet{});
    } catch (const std::exception& e) {
        return folly::makeFuture<std::vector<BindingSet>>(
            folly::make_exception_wrapper<std::runtime_error>(e.what()));
    }
}

} // namespace kbgdb
