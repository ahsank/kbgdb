#include "core/knowledge_base.h"
#include "query/query_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace kbgdb {

// ============================================================================
// Unifier implementation
// ============================================================================

std::string Unifier::resolve(const Term& term, const BindingSet& bindings) {
    if (!term.isVariable()) {
        return term.value;
    }
    
    std::string value = term.value;
    std::string resolved = bindings.get(value);
    
    // Follow the chain of bindings
    while (!resolved.empty()) {
        // Check if resolved value is itself a variable that's bound
        auto next = bindings.get(resolved);
        if (next.empty() || next == resolved) {
            break;
        }
        resolved = next;
    }
    
    return resolved.empty() ? value : resolved;
}

std::optional<BindingSet> Unifier::unifyTerms(
    const std::vector<Term>& terms1,
    const std::vector<Term>& terms2,
    BindingSet bindings) {
    
    if (terms1.size() != terms2.size()) {
        return std::nullopt;
    }

    for (size_t i = 0; i < terms1.size(); ++i) {
        const auto& t1 = terms1[i];
        const auto& t2 = terms2[i];
        
        // Resolve both terms
        std::string val1 = resolve(t1, bindings);
        std::string val2 = resolve(t2, bindings);
        
        bool t1IsVar = t1.isVariable() && bindings.get(t1.value).empty();
        bool t2IsVar = t2.isVariable() && bindings.get(t2.value).empty();
        
        // Check if val1 is still a variable (unbound)
        bool val1IsVar = t1.isVariable() && (val1 == t1.value);
        bool val2IsVar = t2.isVariable() && (val2 == t2.value);
        
        if (val1IsVar && val2IsVar) {
            // Both are unbound variables - bind them together
            if (val1 != val2) {
                bindings.add(val1, val2);
            }
        } else if (val1IsVar) {
            // t1 is unbound variable, bind it to val2
            bindings.add(val1, val2);
        } else if (val2IsVar) {
            // t2 is unbound variable, bind it to val1
            bindings.add(val2, val1);
        } else if (val1 != val2) {
            // Both are ground terms and they don't match
            return std::nullopt;
        }
    }
    
    return bindings;
}

std::optional<BindingSet> Unifier::unify(
    const Fact& goal,
    const Fact& fact,
    const BindingSet& bindings) {
    
    if (goal.predicate() != fact.predicate()) {
        return std::nullopt;
    }
    
    return unifyTerms(goal.terms(), fact.terms(), bindings);
}

Fact Unifier::substitute(const Fact& fact, const BindingSet& bindings) {
    std::vector<Term> newTerms;
    for (const auto& term : fact.terms()) {
        if (term.isVariable()) {
            std::string resolved = resolve(term, bindings);
            if (resolved != term.value) {
                // Variable is bound to a value
                newTerms.push_back(Term{TermType::CONSTANT, resolved});
            } else {
                newTerms.push_back(term);
            }
        } else {
            newTerms.push_back(term);
        }
    }
    return Fact(fact.predicate(), newTerms);
}

// ============================================================================
// KnowledgeBase implementation
// ============================================================================

KnowledgeBase::KnowledgeBase(const std::string& filename) {
    if (!filename.empty()) {
        loadFromFile(filename);
    }
}

void KnowledgeBase::addFact(const Fact& fact) {
    if (fact.predicate_.empty()) {
        std::cerr << "Warning: Attempting to add fact with empty predicate" << std::endl;
        return;
    }
    facts_[fact.predicate()].push_back(fact);
}

void KnowledgeBase::addFact(const std::string& predicate, std::vector<Term> terms) {
    addFact(Fact(predicate, std::move(terms)));
}

const std::vector<Fact>& KnowledgeBase::getFacts(const std::string& predicate) const {
    static const std::vector<Fact> empty;
    auto it = facts_.find(predicate);
    return it != facts_.end() ? it->second : empty;
}

void KnowledgeBase::addRule(const Rule& rule) {
    if (!rule.isValid()) {
        std::cerr << "Warning: Attempting to add invalid rule" << std::endl;
        return;
    }
    rules_.push_back(rule);
}

void KnowledgeBase::addRule(const Fact& head, const std::vector<Fact>& body) {
    addRule(Rule(head, body));
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
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (line[start] == '%') continue;
        
        line = line.substr(start);
        
        // Remove trailing whitespace and period
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            line = line.substr(0, end + 1);
        }
        if (line.empty()) continue;
        
        // Remove trailing period if present
        if (!line.empty() && line.back() == '.') {
            line.pop_back();
        }
        
        try {
            if (line.find(":-") != std::string::npos) {
                // This is a rule
                parser.setRuleMode(true);
                
                size_t pos = line.find(":-");
                std::string headStr = line.substr(0, pos);
                std::string bodyStr = line.substr(pos + 2);
                
                // Trim
                auto trim = [](std::string& s) {
                    size_t start = s.find_first_not_of(" \t");
                    size_t end = s.find_last_not_of(" \t");
                    if (start == std::string::npos) {
                        s.clear();
                    } else {
                        s = s.substr(start, end - start + 1);
                    }
                };
                
                trim(headStr);
                trim(bodyStr);
                
                Fact head = parser.parse(headStr);
                
                // Parse body goals (comma-separated, respecting parentheses)
                std::vector<Fact> body;
                std::string current;
                int parenDepth = 0;
                
                for (size_t i = 0; i < bodyStr.length(); ++i) {
                    char c = bodyStr[i];
                    if (c == '(') {
                        parenDepth++;
                        current += c;
                    } else if (c == ')') {
                        parenDepth--;
                        current += c;
                    } else if (c == ',' && parenDepth == 0) {
                        // End of a goal
                        trim(current);
                        if (!current.empty()) {
                            body.push_back(parser.parse(current));
                        }
                        current.clear();
                    } else {
                        current += c;
                    }
                }
                
                // Don't forget the last goal
                trim(current);
                if (!current.empty()) {
                    body.push_back(parser.parse(current));
                }
                
                addRule(Rule(head, body));
                
            } else {
                // This is a fact
                parser.setRuleMode(true);  // Facts in files use same convention as rules
                addFact(parser.parse(line));
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line: " << line << std::endl;
            std::cerr << "Error: " << e.what() << std::endl;
            throw;
        }
    }
}

std::vector<BindingSet> KnowledgeBase::query(const std::string& queryStr) {
    QueryParser parser;
    parser.setRuleMode(false);  // Query mode: ?X variables
    Fact goal = parser.parse(queryStr);
    return query(goal);
}

std::vector<BindingSet> KnowledgeBase::query(const Fact& goal) {
    std::set<std::string> visited;
    auto results = evaluateGoal(goal, BindingSet{}, visited);
    
    // Extract and resolve only the original query variables
    std::vector<std::string> queryVars;
    for (const auto& term : goal.terms()) {
        if (term.isVariable()) {
            queryVars.push_back(term.value);
        }
    }
    
    // Resolve each result to get final values
    std::vector<BindingSet> resolved;
    for (const auto& binding : results) {
        BindingSet finalBinding;
        for (const auto& var : queryVars) {
            // Follow the chain of bindings to get the final value
            std::string value = var;
            std::set<std::string> seen;
            while (true) {
                if (seen.count(value)) break;  // Cycle detection
                seen.insert(value);
                
                std::string next = binding.get(value);
                if (next.empty()) break;
                value = next;
            }
            // Only add if we found a concrete value (not still a variable)
            // A variable is uppercase or starts with _
            if (!value.empty() && !(std::isupper(value[0]) || value[0] == '_')) {
                finalBinding.add(var, value);
            } else if (!value.empty() && value != var) {
                // It's still a variable, but might be bound in a more complex way
                // Try to get any value
                finalBinding.add(var, value);
            }
        }
        if (!finalBinding.bindings.empty() || queryVars.empty()) {
            resolved.push_back(finalBinding);
        }
    }
    
    return resolved;
}

Rule KnowledgeBase::renameVariables(const Rule& rule, int& counter) const {
    std::unordered_map<std::string, std::string> renaming;
    
    auto renameVar = [&](const std::string& var) -> std::string {
        auto it = renaming.find(var);
        if (it != renaming.end()) {
            return it->second;
        }
        std::string newName = var + "_" + std::to_string(counter++);
        renaming[var] = newName;
        return newName;
    };
    
    auto renameFact = [&](const Fact& f) -> Fact {
        std::vector<Term> newTerms;
        for (const auto& term : f.terms()) {
            if (term.isVariable()) {
                newTerms.push_back(Term{TermType::VARIABLE, renameVar(term.value)});
            } else {
                newTerms.push_back(term);
            }
        }
        return Fact(f.predicate(), newTerms);
    };
    
    Fact newHead = renameFact(rule.head());
    std::vector<Fact> newBody;
    for (const auto& goal : rule.body()) {
        newBody.push_back(renameFact(goal));
    }
    
    return Rule(newHead, newBody);
}

std::vector<BindingSet> KnowledgeBase::evaluateGoal(
    const Fact& goal,
    const BindingSet& bindings,
    std::set<std::string>& visited) const {
    
    // Create a key to detect infinite recursion
    Fact substituted = Unifier::substitute(goal, bindings);
    std::string goalKey = substituted.toString();
    
    // Check for infinite recursion (with same bindings)
    if (visited.count(goalKey)) {
        return {};
    }
    visited.insert(goalKey);
    
    std::vector<BindingSet> results;
    
    // Try to match against facts
    const auto& facts = getFacts(goal.predicate());
    for (const auto& fact : facts) {
        auto unified = Unifier::unify(goal, fact, bindings);
        if (unified) {
            results.push_back(*unified);
        }
    }
    
    // Try to match against rules
    static int varCounter = 0;
    for (const auto& rule : rules_) {
        if (rule.head().predicate() == goal.predicate()) {
            // Rename variables in the rule to avoid capture
            Rule renamedRule = renameVariables(rule, varCounter);
            
            // Try to unify goal with rule head
            auto headBindings = Unifier::unify(goal, renamedRule.head(), bindings);
            if (headBindings) {
                // Evaluate the rule body
                auto bodyResults = evaluateConjunction(
                    renamedRule.body(), *headBindings, visited);
                
                results.insert(results.end(), 
                              bodyResults.begin(), bodyResults.end());
            }
        }
    }
    
    visited.erase(goalKey);
    return results;
}

std::vector<BindingSet> KnowledgeBase::evaluateRule(
    const Rule& rule,
    const Fact& goal,
    const BindingSet& bindings,
    std::set<std::string>& visited) const {
    
    // First unify goal with rule head
    auto headBindings = Unifier::unify(goal, rule.head(), bindings);
    if (!headBindings) {
        return {};
    }
    
    // Then evaluate body goals
    return evaluateConjunction(rule.body(), *headBindings, visited);
}

std::vector<BindingSet> KnowledgeBase::evaluateConjunction(
    const std::vector<Fact>& goals,
    const BindingSet& bindings,
    std::set<std::string>& visited) const {
    
    if (goals.empty()) {
        return {bindings};
    }
    
    // Evaluate first goal
    const Fact& firstGoal = goals[0];
    auto firstResults = evaluateGoal(firstGoal, bindings, visited);
    
    if (goals.size() == 1) {
        return firstResults;
    }
    
    // For each result from first goal, evaluate remaining goals
    std::vector<Fact> remainingGoals(goals.begin() + 1, goals.end());
    std::vector<BindingSet> finalResults;
    
    for (const auto& result : firstResults) {
        auto restResults = evaluateConjunction(remainingGoals, result, visited);
        finalResults.insert(finalResults.end(), 
                           restResults.begin(), restResults.end());
    }
    
    return finalResults;
}

void KnowledgeBase::printFacts() const {
    std::cout << "Facts:" << std::endl;
    for (const auto& [pred, factList] : facts_) {
        for (const auto& fact : factList) {
            std::cout << "  " << fact.toString() << std::endl;
        }
    }
}

void KnowledgeBase::printRules() const {
    std::cout << "Rules:" << std::endl;
    for (const auto& rule : rules_) {
        std::cout << "  " << rule.toString() << std::endl;
    }
}

} // namespace kbgdb
