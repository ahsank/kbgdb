#include "common/fact.h"
#include <sstream>

namespace kbgdb {

std::string Term::toString() const {
    switch (type) {
        case TermType::VARIABLE:
            return "?" + value;
            
        case TermType::CONSTANT:
        case TermType::NUMBER:
            return value;
            
        case TermType::COMPOUND: {
            std::ostringstream oss;
            oss << functor << "(";
            for (size_t i = 0; i < args.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << args[i].toString();
            }
            oss << ")";
            return oss.str();
        }
        
        case TermType::LIST: {
            if (isEmptyList()) {
                return "[]";
            }
            
            // Try to print as [a, b, c] if possible, otherwise [H|T]
            std::ostringstream oss;
            oss << "[";
            
            const Term* current = this;
            bool first = true;
            
            while (current->isConsList()) {
                if (!first) oss << ", ";
                first = false;
                oss << current->head().toString();
                current = &current->tail();
            }
            
            if (current->isEmptyList()) {
                oss << "]";
            } else {
                // Improper list or variable tail: [a, b | T]
                oss << " | " << current->toString() << "]";
            }
            
            return oss.str();
        }
    }
    return "?unknown?";
}

bool Term::operator==(const Term& other) const {
    if (type != other.type) return false;
    
    switch (type) {
        case TermType::VARIABLE:
        case TermType::CONSTANT:
        case TermType::NUMBER:
            return value == other.value;
            
        case TermType::COMPOUND:
            if (functor != other.functor) return false;
            if (args.size() != other.args.size()) return false;
            for (size_t i = 0; i < args.size(); ++i) {
                if (!(args[i] == other.args[i])) return false;
            }
            return true;
            
        case TermType::LIST:
            if (args.size() != other.args.size()) return false;
            for (size_t i = 0; i < args.size(); ++i) {
                if (!(args[i] == other.args[i])) return false;
            }
            return true;
    }
    return false;
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

std::string BindingSet::toString() const {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [var, term] : bindings) {
        if (!first) oss << ", ";
        oss << var << "=" << term.toString();
        first = false;
    }
    oss << "}";
    return oss.str();
}

} // namespace kbgdb
