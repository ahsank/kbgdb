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
    
    std::string toString() const;

    std::string predicate_;
    std::vector<Term> terms_;
    FactSource source_{FactSource::MEMORY};
};

} // namespace kbgdb
