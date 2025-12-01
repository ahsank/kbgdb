#pragma once
#include "common/fact.h"
#include "core/rule.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <set>

namespace kbgdb {

/**
 * Unifier handles the unification algorithm for logic programming.
 * This is a pure utility class with no state.
 */
class Unifier {
public:
    /**
     * Attempt to unify two facts with current bindings.
     * Returns new bindings if successful, nullopt if unification fails.
     */
    static std::optional<BindingSet> unify(
        const Fact& goal,
        const Fact& fact,
        const BindingSet& bindings);
    
    /**
     * Substitute variables in a fact using the given bindings.
     * Returns a new fact with variables replaced by their bound values.
     */
    static Fact substitute(const Fact& fact, const BindingSet& bindings);
    
    /**
     * Get the value of a term, following variable bindings.
     */
    static std::string resolve(const Term& term, const BindingSet& bindings);

private:
    static std::optional<BindingSet> unifyTerms(
        const std::vector<Term>& terms1,
        const std::vector<Term>& terms2,
        BindingSet bindings);
};

/**
 * KnowledgeBase stores facts and rules, and provides synchronous query evaluation.
 * 
 * This is a clean synchronous implementation suitable for in-memory facts and rules.
 * For external data sources, use AsyncKnowledgeBase which wraps this class.
 */
class KnowledgeBase {
public:
    KnowledgeBase() = default;
    explicit KnowledgeBase(const std::string& filename);
    
    // Fact management
    void addFact(const Fact& fact);
    void addFact(const std::string& predicate, std::vector<Term> terms);
    const std::vector<Fact>& getFacts(const std::string& predicate) const;
    
    // Rule management
    void addRule(const Rule& rule);
    void addRule(const Fact& head, const std::vector<Fact>& body);
    const std::vector<Rule>& getRules() const { return rules_; }
    
    // Loading from file
    void loadFromFile(const std::string& filename);
    
    // Synchronous query interface
    std::vector<BindingSet> query(const std::string& queryStr);
    std::vector<BindingSet> query(const Fact& goal);
    
    // Debug/info
    void printFacts() const;
    void printRules() const;

private:
    std::vector<Rule> rules_;
    std::unordered_map<std::string, std::vector<Fact>> facts_;
    
    // Core evaluation - synchronous
    std::vector<BindingSet> evaluateGoal(
        const Fact& goal,
        const BindingSet& bindings,
        std::set<std::string>& visited) const;
    
    std::vector<BindingSet> evaluateRule(
        const Rule& rule,
        const Fact& goal,
        const BindingSet& bindings,
        std::set<std::string>& visited) const;
    
    std::vector<BindingSet> evaluateConjunction(
        const std::vector<Fact>& goals,
        const BindingSet& bindings,
        std::set<std::string>& visited) const;
    
    // Rename variables in a rule to avoid capture
    Rule renameVariables(const Rule& rule, int& counter) const;
};

} // namespace kbgdb
