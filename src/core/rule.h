#pragma once
#include "common/fact.h"
#include <vector>

namespace kbgdb {

class Rule {
public:
    Rule(Fact head, std::vector<Fact> body);
    
    const Fact& getHead() const { return head_; }
    const std::vector<Fact>& getBody() const { return body_; }
    
    std::string toString() const;
    bool isValid() const;

private:
    Fact head_;
    std::vector<Fact> body_;
};

} // namespace kbgdb
