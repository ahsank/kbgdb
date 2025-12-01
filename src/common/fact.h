#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace kbgdb {

/**
 * Term types in our logic system.
 * 
 * VARIABLE: Represents a logic variable that can be bound to values.
 *           Internally stored WITHOUT the '?' prefix.
 *           - In queries: written as ?X, ?Name, etc.
 *           - In rules: written as X, Name, _X, _ (uppercase or underscore prefix)
 * 
 * CONSTANT: Represents a fixed atom/symbol (lowercase).
 * NUMBER: Represents a numeric value.
 */
enum class TermType {
    VARIABLE,
    CONSTANT,
    NUMBER
};

struct Term {
    TermType type;
    std::string value;  // For variables, stored WITHOUT '?' prefix
    
    bool isVariable() const { return type == TermType::VARIABLE; }
    bool isConstant() const { return type == TermType::CONSTANT; }
    bool isNumber() const { return type == TermType::NUMBER; }
    
    std::string toString() const;
    
    bool operator==(const Term& other) const {
        return type == other.type && value == other.value;
    }
};

/**
 * A Fact represents a predicate applied to terms.
 * Examples: parent(john, mary), age(bob, 10), grandparent(X, Y)
 */
class Fact {
public:
    Fact() = default;
    Fact(std::string predicate, std::vector<Term> terms);
    
    const std::string& predicate() const { return predicate_; }
    const std::vector<Term>& terms() const { return terms_; }
    size_t arity() const { return terms_.size(); }
    
    std::string toString() const;
    
    bool operator==(const Fact& other) const {
        return predicate_ == other.predicate_ && terms_ == other.terms_;
    }

    std::string predicate_;
    std::vector<Term> terms_;
};

/**
 * BindingSet holds variable bindings during unification and evaluation.
 * Keys are variable names WITHOUT the '?' prefix.
 */
struct BindingSet {
    std::unordered_map<std::string, std::string> bindings;
    
    void add(const std::string& var, const std::string& value) {
        bindings[var] = value;
    }
    
    std::string get(const std::string& var) const {
        auto it = bindings.find(var);
        return it != bindings.end() ? it->second : "";
    }
    
    bool has(const std::string& var) const {
        return bindings.find(var) != bindings.end();
    }
    
    bool empty() const { return bindings.empty(); }
    
    std::string toString() const;
};

} // namespace kbgdb
