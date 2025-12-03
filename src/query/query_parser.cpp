#include "query/query_parser.h"
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace kbgdb {

Fact QueryParser::parse(const std::string& input) {
    tokens_ = tokenize(input);
    current_ = 0;

    if (tokens_.empty()) {
        throw std::runtime_error("Empty query");
    }

    // Parse predicate
    if (!match(Token::IDENTIFIER)) {
        throw std::runtime_error("Expected predicate name");
    }
    std::string predicate = tokens_[current_ - 1].value;

    // Parse opening parenthesis
    if (!match(Token::LPAREN)) {
        throw std::runtime_error("Expected '(' after predicate");
    }

    // Parse terms
    std::vector<Term> terms;
    while (!isAtEnd() && !check(Token::RPAREN)) {
        terms.push_back(parseTermInternal());
        
        if (!isAtEnd() && !check(Token::RPAREN)) {
            if (!match(Token::COMMA)) {
                throw std::runtime_error("Expected ',' between terms");
            }
        }
    }

    // Parse closing parenthesis
    if (!match(Token::RPAREN)) {
        throw std::runtime_error("Expected ')' after terms");
    }

    return Fact(predicate, terms);
}

Term QueryParser::parseTerm(const std::string& input) {
    tokens_ = tokenize(input);
    current_ = 0;
    
    if (tokens_.empty()) {
        throw std::runtime_error("Empty term");
    }
    
    return parseTermInternal();
}

std::vector<QueryParser::Token> QueryParser::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    std::string current;
    
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        
        if (std::isspace(c)) {
            if (!current.empty()) {
                addToken(tokens, current);
                current.clear();
            }
            continue;
        }
        
        // Handle special characters
        if (c == '(' || c == ')' || c == ',' || c == '[' || c == ']' || c == '|') {
            if (!current.empty()) {
                addToken(tokens, current);
                current.clear();
            }
            Token token;
            switch (c) {
                case '(': token.type = Token::LPAREN; break;
                case ')': token.type = Token::RPAREN; break;
                case ',': token.type = Token::COMMA; break;
                case '[': token.type = Token::LBRACKET; break;
                case ']': token.type = Token::RBRACKET; break;
                case '|': token.type = Token::PIPE; break;
            }
            tokens.push_back(token);
            continue;
        }
        
        current += c;
    }
    
    if (!current.empty()) {
        addToken(tokens, current);
    }
    
    return tokens;
}

void QueryParser::addToken(std::vector<Token>& tokens, const std::string& value) {
    Token token;
    
    if (!ruleMode_ && !value.empty() && value[0] == '?') {
        // Query mode: variables start with '?'
        token.type = Token::VARIABLE;
        token.value = value.substr(1);  // Strip '?'
    } else if (ruleMode_ && !value.empty() && 
               (std::isupper(value[0]) || value[0] == '_')) {
        // Rule mode: variables are uppercase or start with underscore
        token.type = Token::VARIABLE;
        token.value = value;
    } else if (!value.empty() && (std::isdigit(value[0]) || 
               (value[0] == '-' && value.length() > 1 && std::isdigit(value[1])))) {
        token.type = Token::NUMBER;
        token.value = value;
    } else {
        token.type = Token::IDENTIFIER;
        token.value = value;
    }
    
    tokens.push_back(token);
}

Term QueryParser::parseTermInternal() {
    if (isAtEnd()) {
        throw std::runtime_error("Unexpected end of input while parsing term");
    }
    
    // Check for list
    if (check(Token::LBRACKET)) {
        return parseList();
    }
    
    return parseCompoundOrAtom();
}

Term QueryParser::parseList() {
    consume(Token::LBRACKET);
    
    // Empty list
    if (match(Token::RBRACKET)) {
        return Term::emptyList();
    }
    
    // Parse list elements
    std::vector<Term> elements;
    elements.push_back(parseTermInternal());
    
    while (match(Token::COMMA)) {
        elements.push_back(parseTermInternal());
    }
    
    // Check for tail: [a, b | Tail]
    if (match(Token::PIPE)) {
        Term tail = parseTermInternal();
        consume(Token::RBRACKET);
        
        // Build list from back: elements... | tail
        Term result = tail;
        for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
            result = Term::cons(*it, std::move(result));
        }
        return result;
    }
    
    consume(Token::RBRACKET);
    
    // Build proper list ending with []
    return Term::list(std::move(elements));
}

Term QueryParser::parseCompoundOrAtom() {
    Token token = advance();
    
    switch (token.type) {
        case Token::VARIABLE:
            return Term::variable(token.value);
            
        case Token::NUMBER:
            return Term::number(token.value);
            
        case Token::IDENTIFIER: {
            // Check if it's a compound term: functor(args...)
            if (match(Token::LPAREN)) {
                std::vector<Term> args;
                
                if (!check(Token::RPAREN)) {
                    args.push_back(parseTermInternal());
                    while (match(Token::COMMA)) {
                        args.push_back(parseTermInternal());
                    }
                }
                
                consume(Token::RPAREN);
                return Term::compound(token.value, std::move(args));
            }
            
            // Simple atom
            return Term::constant(token.value);
        }
            
        default:
            throw std::runtime_error("Unexpected token type in term");
    }
}

QueryParser::Token QueryParser::consume(Token::Type expected) {
    if (check(expected)) {
        return advance();
    }
    std::string msg = "Expected token type " + std::to_string(expected) + 
                      " but got " + std::to_string(peek().type);
    if (!isAtEnd()) {
        msg += " with value '" + peek().value + "'";
    }
    throw std::runtime_error(msg);
}

bool QueryParser::check(Token::Type type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool QueryParser::match(Token::Type type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

QueryParser::Token QueryParser::peek() const {
    if (isAtEnd()) {
        return Token{Token::END, ""};
    }
    return tokens_[current_];
}

QueryParser::Token QueryParser::advance() {
    if (!isAtEnd()) current_++;
    return tokens_[current_ - 1];
}

bool QueryParser::isAtEnd() const {
    return current_ >= tokens_.size();
}

} // namespace kbgdb
