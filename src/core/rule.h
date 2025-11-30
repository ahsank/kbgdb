#pragma once
#include "common/fact.h"
#include <vector>

namespace kbgdb {

class Rule {
public:
    std::string toString() const;
    bool isValid() const;
    Fact head_;
    std::vector<Fact> body_;
};

} // namespace kbgdb
