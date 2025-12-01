#pragma once
#include "common/fact.h"
#include <string>
#include <vector>

namespace kbgdb {

/**
 * QueryParser parses logic queries and rule definitions.
 * 
 * Variable conventions:
 * - Query mode (default): Variables start with '?' (e.g., ?X, ?Name)
 *   The '?' is stripped when storing the variable.
 * 
 * - Rule mode: Variables are uppercase letters or start with '_'
 *   (e.g., X, Name, _X, _)
 * 
 * Constants are lowercase identifiers.
 * Numbers are sequences of digits.
 */
class QueryParser {
public:
    /**
     * Parse a fact/query string.
     * Examples:
     *   Query mode:  "parent(?X, mary)" -> parent with var X and const mary
     *   Rule mode:   "parent(X, mary)"  -> parent with var X and const mary
     */
    Fact parse(const std::string& input);
    
    /**
     * Set whether we're parsing rule definitions (uppercase vars)
     * or queries (?-prefixed vars).
     */
    void setRuleMode(bool isRule) { ruleMode_ = isRule; }
    bool isRuleMode() const { return ruleMode_; }

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
    
    bool ruleMode_ = false;
    std::vector<Token> tokens_;
    size_t current_ = 0;

    std::vector<Token> tokenize(const std::string& input);
    void addToken(std::vector<Token>& tokens, const std::string& value);
    Term parseTerm();
    
    Token consume(Token::Type expected);
    bool check(Token::Type type) const;
    bool match(Token::Type type);
    Token peek() const;
    Token advance();
    bool isAtEnd() const;
};

} // namespace kbgdb
