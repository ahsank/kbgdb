// src/query/query_parser.h
#pragma once
#include "common/fact.h"
#include <string>
#include <vector>

namespace kbgdb {

class QueryParser {
public:
    Fact parse(const std::string& query_str);
    // Add a method to set rule parsing mode
    void setRuleMode(bool isRule) {
        isRuleMode_ = isRule;
    }

private:
    struct Token {
        enum Type {
            IDENTIFIER,
            VARIABLE,
            NUMBER,
            LPAREN,
            RPAREN,
            COMMA
        };
        Type type;
        std::string value;
    };
    bool isRuleMode_ = false; 
    std::vector<Token> tokens_;
    size_t current_ = 0;

    std::vector<Token> tokenize(const std::string& input);
    void addToken(std::vector<Token>& tokens, const std::string& value);
    Token consume(Token::Type expected);
    Term parseTerm();
    bool check(Token::Type type) const;
    bool match(Token::Type type);
    Token peek() const;
    Token advance();
    bool isAtEnd() const;
};

} // namespace kbgdb
