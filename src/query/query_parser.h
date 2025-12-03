#pragma once
#include "common/fact.h"
#include <string>
#include <vector>

namespace kbgdb {

/**
 * QueryParser parses logic queries and rule definitions.
 * 
 * Supports:
 * - Simple terms: atoms, numbers, variables
 * - Compound terms: f(X, Y), point(1, 2)
 * - Lists: [], [1, 2, 3], [H|T], [a, b | Rest]
 * 
 * Variable conventions:
 * - Query mode (default): Variables start with '?' (e.g., ?X, ?Name)
 * - Rule mode: Variables are uppercase or start with '_' (e.g., X, _X, _)
 */
class QueryParser {
public:
    /**
     * Parse a fact/query string.
     */
    Fact parse(const std::string& input);
    
    /**
     * Parse a single term (for testing or standalone use)
     */
    Term parseTerm(const std::string& input);
    
    /**
     * Set whether we're parsing rule definitions (uppercase vars)
     * or queries (?-prefixed vars).
     */
    void setRuleMode(bool isRule) { ruleMode_ = isRule; }
    bool isRuleMode() const { return ruleMode_; }

private:
    struct Token {
        enum Type {
            IDENTIFIER,     // lowercase atom
            VARIABLE,       // variable
            NUMBER,         // numeric literal
            LPAREN,         // (
            RPAREN,         // )
            LBRACKET,       // [
            RBRACKET,       // ]
            PIPE,           // |
            COMMA,          // ,
            END             // end of input
        };
        Type type;
        std::string value;
    };
    
    bool ruleMode_ = false;
    std::vector<Token> tokens_;
    size_t current_ = 0;

    std::vector<Token> tokenize(const std::string& input);
    void addToken(std::vector<Token>& tokens, const std::string& value);
    
    Term parseTermInternal();
    Term parseList();
    Term parseCompoundOrAtom();
    
    Token consume(Token::Type expected);
    bool check(Token::Type type) const;
    bool match(Token::Type type);
    Token peek() const;
    Token advance();
    bool isAtEnd() const;
};

} // namespace kbgdb
