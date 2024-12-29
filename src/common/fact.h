#pragma once
#include <string>
#include <vector>

namespace kbgdb {

enum class TermType {
    VARIABLE,
    CONSTANT,
    NUMBER
};

struct Term {
    TermType type;
    std::string value;
    
    bool isVariable() const { return type == TermType::VARIABLE; }
    std::string toString() const;
};

enum class FactSource {
    MEMORY,
    ROCKSDB
};

class Fact {
public:
    Fact(std::string predicate, std::vector<Term> terms);
    
    const std::string& getPredicate() const { return predicate_; }
    const std::vector<Term>& getTerms() const { return terms_; }
    FactSource getSource() const { return source_; }
    
    void setSource(FactSource source) { source_ = source; }
    std::string toString() const;

private:
    std::string predicate_;
    std::vector<Term> terms_;
    FactSource source_{FactSource::MEMORY};
};

} // namespace kbgdb
