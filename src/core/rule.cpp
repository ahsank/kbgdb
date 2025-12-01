#include "core/rule.h"
#include <sstream>

namespace kbgdb {

Rule::Rule(Fact head, std::vector<Fact> body)
    : head_(std::move(head))
    , body_(std::move(body)) {
}

std::string Rule::toString() const {
    std::ostringstream oss;
    oss << head_.toString();
    oss << " :- ";
    for (size_t i = 0; i < body_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << body_[i].toString();
    }
    oss << ".";
    return oss.str();
}

bool Rule::isValid() const {
    return !head_.predicate_.empty() && !body_.empty();
}

} // namespace kbgdb
