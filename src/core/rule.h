#pragma once
#include "common/fact.h"
#include <vector>

namespace kbgdb {

/**
 * A Rule represents a logical implication: head :- body1, body2, ...
 * 
 * Example: grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
 * 
 * The head is the conclusion, the body contains the premises.
 * Variables in rules are uppercase letters or start with underscore.
 */
class Rule {
public:
    Rule() = default;
    Rule(Fact head, std::vector<Fact> body);
    
    const Fact& head() const { return head_; }
    const std::vector<Fact>& body() const { return body_; }
    
    std::string toString() const;
    bool isValid() const;

    Fact head_;
    std::vector<Fact> body_;
};

} // namespace kbgdb
