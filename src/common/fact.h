#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

namespace kbgdb {

/**
 * Term types in our logic system.
 * 
 * VARIABLE: Logic variable (e.g., X, ?X)
 * CONSTANT: Atom/symbol (e.g., john, foo)
 * NUMBER: Numeric value (e.g., 42, 3.14)
 * COMPOUND: Compound term with functor and args (e.g., f(X, Y), point(1, 2))
 * LIST: List structure - either empty [] or cons [H|T]
 */
enum class TermType {
    VARIABLE,
    CONSTANT,
    NUMBER,
    COMPOUND,
    LIST
};

struct Term {
    TermType type;
    std::string value;              // For VARIABLE, CONSTANT, NUMBER
    std::string functor;            // For COMPOUND terms (e.g., "f" in f(X,Y))
    std::vector<Term> args;         // For COMPOUND and LIST (cons cell: [head, tail])
    
    // Constructors
    Term() : type(TermType::CONSTANT) {}
    Term(TermType t, std::string v) : type(t), value(std::move(v)) {}
    
    // Type checks
    bool isVariable() const { return type == TermType::VARIABLE; }
    bool isConstant() const { return type == TermType::CONSTANT; }
    bool isNumber() const { return type == TermType::NUMBER; }
    bool isCompound() const { return type == TermType::COMPOUND; }
    bool isList() const { return type == TermType::LIST; }
    
    // List helpers
    bool isEmptyList() const { return type == TermType::LIST && args.empty(); }
    bool isConsList() const { return type == TermType::LIST && args.size() == 2; }
    
    const Term& head() const { return args[0]; }  // For cons list [H|T]
    const Term& tail() const { return args[1]; }  // For cons list [H|T]
    
    // Factory methods for cleaner construction
    static Term variable(const std::string& name) {
        return Term{TermType::VARIABLE, name};
    }
    
    static Term constant(const std::string& name) {
        return Term{TermType::CONSTANT, name};
    }
    
    static Term number(const std::string& val) {
        return Term{TermType::NUMBER, val};
    }
    
    static Term compound(const std::string& functor, std::vector<Term> args) {
        Term t;
        t.type = TermType::COMPOUND;
        t.functor = functor;
        t.args = std::move(args);
        return t;
    }
    
    static Term emptyList() {
        Term t;
        t.type = TermType::LIST;
        // args is empty for []
        return t;
    }
    
    static Term cons(Term head, Term tail) {
        Term t;
        t.type = TermType::LIST;
        t.args.push_back(std::move(head));
        t.args.push_back(std::move(tail));
        return t;
    }
    
    // Build a list from elements: [1, 2, 3] -> cons(1, cons(2, cons(3, [])))
    static Term list(std::vector<Term> elements) {
        Term result = emptyList();
        for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
            result = cons(*it, std::move(result));
        }
        return result;
    }
    
    std::string toString() const;
    
    bool operator==(const Term& other) const;
};

/**
 * A Fact represents a predicate applied to terms.
 * Examples: parent(john, mary), age(bob, 10), member(X, [1,2,3])
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
 * Now supports binding to full Term objects, not just strings.
 */
struct BindingSet {
    std::unordered_map<std::string, Term> bindings;
    
    void add(const std::string& var, const Term& value) {
        bindings[var] = value;
    }
    
    // Legacy string-based add (for simple constants)
    void add(const std::string& var, const std::string& value) {
        bindings[var] = Term::constant(value);
    }
    
    std::optional<Term> getTerm(const std::string& var) const {
        auto it = bindings.find(var);
        if (it != bindings.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    // Legacy string-based get (returns empty if not found or not a simple value)
    std::string get(const std::string& var) const {
        auto it = bindings.find(var);
        if (it != bindings.end()) {
            const Term& t = it->second;
            if (t.isConstant() || t.isNumber() || t.isVariable()) {
                return t.value;
            }
        }
        return "";
    }
    
    bool has(const std::string& var) const {
        return bindings.find(var) != bindings.end();
    }
    
    bool empty() const { return bindings.empty(); }
    
    std::string toString() const;
};

} // namespace kbgdb
