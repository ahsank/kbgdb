// src/query/query_parser.cpp
#include "query/query_parser.h"
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace kbgdb {
Fact QueryParser::parse(const std::string& query_str) {
    current_ = 0;
    tokens_ = tokenize(query_str);

    // Parse predicate
    auto predicateToken = consume(Token::IDENTIFIER);
    std::string predicate = predicateToken.value;

    // Expect opening parenthesis
    consume(Token::LPAREN);

    // Parse terms
    std::vector<Term> terms;
    while (!isAtEnd() && !check(Token::RPAREN)) {
        if (!terms.empty()) {
            consume(Token::COMMA);
        }
        terms.push_back(parseTerm());
    }

    // Expect closing parenthesis
    if (!isAtEnd()) {
        consume(Token::RPAREN);
    }

    return Fact(predicate, terms);
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
        
        // Handle parentheses and commas
        if (c == '(' || c == ')' || c == ',') {
            if (!current.empty()) {
                addToken(tokens, current);
                current.clear();
            }
            Token token;
            token.type = c == '(' ? Token::LPAREN :
                        c == ')' ? Token::RPAREN : 
                        Token::COMMA;
            tokens.push_back(token);
            continue;
        }
        
        current += c;
    }
    
    if (!current.empty()) {
        addToken(tokens, current);
    }

    // Debug output
    std::cout << "Tokenized: ";
    for (const auto& token : tokens) {
        std::cout << "(" << token.type << ":" << token.value << ") ";
    }
    std::cout << std::endl;
    
    return tokens;
}

void QueryParser::addToken(std::vector<Token>& tokens, const std::string& value) {
    Token token;
    if (!isRuleMode_ && value[0] == '?') {
        // Query mode: variables start with ?
        token.type = Token::VARIABLE;
        token.value = value.substr(1);
    } else if (isRuleMode_ && (std::isupper(value[0]) || value[0] == '_')) {
        // Rule mode: variables are uppercase or start with underscore
        token.type = Token::VARIABLE;
        token.value = value;
    } else if (std::isdigit(value[0])) {
        token.type = Token::NUMBER;
        token.value = value;
    } else {
        token.type = Token::IDENTIFIER;
        token.value = value;
    }
    tokens.push_back(token);
}

Term QueryParser::parseTerm() {
    Token token = advance();
    Term term;
    
    switch (token.type) {
        case Token::VARIABLE:
            term.type = TermType::VARIABLE;
            term.value = token.value;
            break;
        case Token::NUMBER:
            term.type = TermType::NUMBER;
            term.value = token.value;
            break;
        case Token::IDENTIFIER:
            term.type = TermType::CONSTANT;
            term.value = token.value;
            break;
        default:
            throw std::runtime_error("Unexpected token type in term");
    }
    
    return term;
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
