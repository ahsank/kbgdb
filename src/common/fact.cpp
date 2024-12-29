#include "common/fact.h"
#include <sstream>

namespace kbgdb {

std::string Term::toString() const {
    if (type == TermType::VARIABLE) {
        return "?" + value;
    }
    return value;
}

Fact::Fact(std::string predicate, std::vector<Term> terms)
    : predicate_(std::move(predicate))
    , terms_(std::move(terms)) {
}

std::string Fact::toString() const {
    std::ostringstream oss;
    oss << predicate_ << "(";
    for (size_t i = 0; i < terms_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << terms_[i].toString();
    }
    oss << ")";
    return oss.str();
}

} // namespace kbgdb
