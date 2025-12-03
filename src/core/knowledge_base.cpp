#include "core/knowledge_base.h"
#include "query/query_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <set>
#include <functional>

namespace kbgdb {

// ============================================================================
// Unifier implementation - supports compound terms and lists
// Following Norvig's PAIP approach
// ============================================================================

// Forward declarations
static std::optional<BindingSet> unifyImpl(
    const Term& x, const Term& y, BindingSet bindings);
static std::optional<BindingSet> unifyVariable(
    const Term& var, const Term& x, BindingSet bindings);
static bool occursCheck(
    const std::string& var, const Term& x, const BindingSet& bindings);
static Term resolveTerm(const Term& term, const BindingSet& bindings);

/**
 * Check if a string looks like a variable name
 */
static bool isVariableName(const std::string& s) {
    if (s.empty()) return false;
    return std::isupper(s[0]) || s[0] == '_';
}

/**
 * Fully resolve a term, following variable bindings recursively.
 * Returns a term with all bound variables replaced by their values.
 */
static Term resolveTerm(const Term& term, const BindingSet& bindings) {
    switch (term.type) {
        case TermType::VARIABLE: {
            auto bound = bindings.getTerm(term.value);
            if (bound) {
                // Recursively resolve the bound term
                return resolveTerm(*bound, bindings);
            }
            return term;  // Unbound variable
        }
        
        case TermType::CONSTANT:
        case TermType::NUMBER:
            return term;
        
        case TermType::COMPOUND: {
            std::vector<Term> resolvedArgs;
            for (const auto& arg : term.args) {
                resolvedArgs.push_back(resolveTerm(arg, bindings));
            }
            return Term::compound(term.functor, std::move(resolvedArgs));
        }
        
        case TermType::LIST: {
            if (term.isEmptyList()) {
                return term;
            }
            // Cons cell
            Term head = resolveTerm(term.head(), bindings);
            Term tail = resolveTerm(term.tail(), bindings);
            return Term::cons(std::move(head), std::move(tail));
        }
    }
    return term;
}

/**
 * Occurs check: Does var occur anywhere inside x?
 * Prevents infinite structures like X = f(X)
 */
static bool occursCheck(
    const std::string& var, const Term& x, const BindingSet& bindings) {
    
    switch (x.type) {
        case TermType::VARIABLE: {
            if (x.value == var) return true;
            
            // If x is bound, check what it's bound to
            auto bound = bindings.getTerm(x.value);
            if (bound) {
                return occursCheck(var, *bound, bindings);
            }
            return false;
        }
        
        case TermType::CONSTANT:
        case TermType::NUMBER:
            return false;
        
        case TermType::COMPOUND:
        case TermType::LIST: {
            for (const auto& arg : x.args) {
                if (occursCheck(var, arg, bindings)) {
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

/**
 * Unify a variable with a term x.
 * Following Norvig's unify-variable from PAIP.
 */
static std::optional<BindingSet> unifyVariable(
    const Term& var, const Term& x, BindingSet bindings) {
    
    // If var is already bound, unify its value with x
    auto varBinding = bindings.getTerm(var.value);
    if (varBinding) {
        return unifyImpl(*varBinding, x, bindings);
    }
    
    // If x is a variable that's bound, unify var with x's value
    if (x.isVariable()) {
        auto xBinding = bindings.getTerm(x.value);
        if (xBinding) {
            return unifyImpl(var, *xBinding, bindings);
        }
    }
    
    // Occurs check: prevent X = f(X) style infinite structures
    if (occursCheck(var.value, x, bindings)) {
        return std::nullopt;
    }
    
    // Extend bindings: bind var to x
    bindings.add(var.value, x);
    return bindings;
}

/**
 * Core unification: See if x and y match with given bindings.
 * Following Norvig's unify from PAIP.
 */
static std::optional<BindingSet> unifyImpl(
    const Term& x, const Term& y, BindingSet bindings) {
    
    // If x and y are identical atoms/numbers/empty lists
    if (x.type == y.type) {
        if ((x.isConstant() || x.isNumber()) && x.value == y.value) {
            return bindings;
        }
        if (x.isVariable() && x.value == y.value) {
            return bindings;
        }
        if (x.isEmptyList() && y.isEmptyList()) {
            return bindings;
        }
    }
    
    // If x is a variable, use unify-variable
    if (x.isVariable()) {
        return unifyVariable(x, y, bindings);
    }
    
    // If y is a variable, use unify-variable (symmetric)
    if (y.isVariable()) {
        return unifyVariable(y, x, bindings);
    }
    
    // Both are compound terms
    if (x.isCompound() && y.isCompound()) {
        if (x.functor != y.functor || x.args.size() != y.args.size()) {
            return std::nullopt;
        }
        for (size_t i = 0; i < x.args.size(); ++i) {
            auto result = unifyImpl(x.args[i], y.args[i], bindings);
            if (!result) return std::nullopt;
            bindings = *result;
        }
        return bindings;
    }
    
    // Both are lists (cons cells)
    if (x.isList() && y.isList()) {
        // Both empty - already handled above
        if (x.isEmptyList() || y.isEmptyList()) {
            // One empty, one not - fail
            return std::nullopt;
        }
        // Both are cons cells - unify head and tail
        auto headResult = unifyImpl(x.head(), y.head(), bindings);
        if (!headResult) return std::nullopt;
        return unifyImpl(x.tail(), y.tail(), *headResult);
    }
    
    // Type mismatch - fail
    return std::nullopt;
}

// ============================================================================
// Unifier public interface
// ============================================================================

std::string Unifier::resolve(const Term& term, const BindingSet& bindings) {
    Term resolved = resolveTerm(term, bindings);
    if (resolved.isConstant() || resolved.isNumber()) {
        return resolved.value;
    }
    if (resolved.isVariable()) {
        return resolved.value;
    }
    // For compound/list, return toString representation
    return resolved.toString();
}

std::pair<std::string, bool> Unifier::resolveWithType(
    const Term& term, const BindingSet& bindings) {
    
    Term resolved = resolveTerm(term, bindings);
    if (resolved.isVariable()) {
        return {resolved.value, true};
    }
    if (resolved.isConstant() || resolved.isNumber()) {
        return {resolved.value, false};
    }
    // Compound/list - return toString, not a variable
    return {resolved.toString(), false};
}

Term Unifier::resolveFull(const Term& term, const BindingSet& bindings) {
    return resolveTerm(term, bindings);
}

std::optional<BindingSet> Unifier::unifyTerms(
    const std::vector<Term>& terms1,
    const std::vector<Term>& terms2,
    BindingSet bindings) {
    
    if (terms1.size() != terms2.size()) {
        return std::nullopt;
    }

    for (size_t i = 0; i < terms1.size(); ++i) {
        auto result = unifyImpl(terms1[i], terms2[i], bindings);
        if (!result) return std::nullopt;
        bindings = *result;
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

Term Unifier::substituteTerm(const Term& term, const BindingSet& bindings) {
    return resolveTerm(term, bindings);
}

Fact Unifier::substitute(const Fact& fact, const BindingSet& bindings) {
    std::vector<Term> newTerms;
    for (const auto& term : fact.terms()) {
        newTerms.push_back(resolveTerm(term, bindings));
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
                
                // Parse body goals (comma-separated, respecting parentheses and brackets)
                std::vector<Fact> body;
                std::string current;
                int parenDepth = 0;
                int bracketDepth = 0;
                
                for (size_t i = 0; i < bodyStr.length(); ++i) {
                    char c = bodyStr[i];
                    if (c == '(') {
                        parenDepth++;
                        current += c;
                    } else if (c == ')') {
                        parenDepth--;
                        current += c;
                    } else if (c == '[') {
                        bracketDepth++;
                        current += c;
                    } else if (c == ']') {
                        bracketDepth--;
                        current += c;
                    } else if (c == ',' && parenDepth == 0 && bracketDepth == 0) {
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
    
    // Collect all query variables (including those inside compound terms/lists)
    std::vector<std::string> queryVars;
    std::function<void(const Term&)> collectVars;
    collectVars = [&](const Term& term) {
        switch (term.type) {
            case TermType::VARIABLE:
                queryVars.push_back(term.value);
                break;
            case TermType::COMPOUND:
            case TermType::LIST:
                for (const auto& arg : term.args) {
                    collectVars(arg);
                }
                break;
            default:
                break;
        }
    };
    
    for (const auto& term : goal.terms()) {
        collectVars(term);
    }
    
    // Resolve each result to get final values
    std::vector<BindingSet> resolved;
    for (const auto& binding : results) {
        BindingSet finalBinding;
        for (const auto& var : queryVars) {
            // Use the new resolveFull to get the complete resolved term
            Term varTerm = Term::variable(var);
            Term resolvedTerm = Unifier::resolveFull(varTerm, binding);
            
            // Only add if we found a non-variable value
            if (!resolvedTerm.isVariable()) {
                finalBinding.add(var, resolvedTerm);
            } else if (resolvedTerm.value != var) {
                // Bound to another variable
                finalBinding.add(var, resolvedTerm);
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
    
    // Forward declaration for recursion
    std::function<Term(const Term&)> renameTerm;
    
    renameTerm = [&](const Term& term) -> Term {
        switch (term.type) {
            case TermType::VARIABLE:
                return Term::variable(renameVar(term.value));
                
            case TermType::CONSTANT:
            case TermType::NUMBER:
                return term;
                
            case TermType::COMPOUND: {
                std::vector<Term> newArgs;
                for (const auto& arg : term.args) {
                    newArgs.push_back(renameTerm(arg));
                }
                return Term::compound(term.functor, std::move(newArgs));
            }
            
            case TermType::LIST: {
                if (term.isEmptyList()) {
                    return term;
                }
                return Term::cons(
                    renameTerm(term.head()),
                    renameTerm(term.tail())
                );
            }
        }
        return term;
    };
    
    auto renameFact = [&](const Fact& f) -> Fact {
        std::vector<Term> newTerms;
        for (const auto& term : f.terms()) {
            newTerms.push_back(renameTerm(term));
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
