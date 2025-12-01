// examples/repl.cpp
// Interactive Read-Eval-Print-Loop for the logic engine

#include "core/knowledge_base.h"
#include "query/query_engine.h"
#include "query/query_parser.h"
#include <iostream>
#include <string>
#include <sstream>

void printHelp() {
    std::cout << R"(
KBGDB Interactive REPL
======================

Commands:
  ?- query          Execute a query (e.g., ?- parent(?X, mary))
  assert fact       Add a fact (e.g., assert parent(john, bob))
  rule head :- body Add a rule (e.g., rule grandparent(X,Z) :- parent(X,Y), parent(Y,Z))
  facts             List all facts
  rules             List all rules
  load <file>       Load facts/rules from file
  help              Show this help
  quit              Exit the REPL

Variable conventions:
  - In queries: use ?X, ?Name, etc.
  - In rules:   use X, Name, _X, etc. (uppercase or underscore prefix)

Examples:
  ?- parent(?X, mary)
  ?- grandparent(john, ?Y)
  assert person(alice)
  rule sibling(X,Y) :- parent(Z,X), parent(Z,Y)
)" << std::endl;
}

int main(int argc, char* argv[]) {
    kbgdb::KnowledgeBase kb;
    kbgdb::QueryEngine engine(std::make_shared<kbgdb::KnowledgeBase>(kb));
    kbgdb::QueryParser parser;
    
    // Load initial file if provided
    if (argc > 1) {
        try {
            std::cout << "Loading: " << argv[1] << std::endl;
            kb.loadFromFile(argv[1]);
            std::cout << "Loaded successfully." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error loading file: " << e.what() << std::endl;
        }
    }
    
    std::cout << "KBGDB Interactive REPL (type 'help' for commands)" << std::endl;
    std::cout << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "kbgdb> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        // Trim whitespace
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        size_t end = line.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            line = line.substr(0, end + 1);
        }
        
        if (line.empty()) continue;
        
        try {
            if (line == "quit" || line == "exit") {
                break;
            } else if (line == "help") {
                printHelp();
            } else if (line == "facts") {
                kb.printFacts();
            } else if (line == "rules") {
                kb.printRules();
            } else if (line.substr(0, 5) == "load ") {
                std::string filename = line.substr(5);
                kb.loadFromFile(filename);
                std::cout << "Loaded: " << filename << std::endl;
            } else if (line.substr(0, 7) == "assert ") {
                std::string factStr = line.substr(7);
                parser.setRuleMode(true);
                kbgdb::Fact fact = parser.parse(factStr);
                kb.addFact(fact);
                std::cout << "Asserted: " << fact.toString() << std::endl;
            } else if (line.substr(0, 5) == "rule ") {
                std::string ruleStr = line.substr(5);
                size_t pos = ruleStr.find(":-");
                if (pos == std::string::npos) {
                    std::cerr << "Invalid rule syntax. Use: rule head(X) :- body(X)" << std::endl;
                    continue;
                }
                
                std::string headStr = ruleStr.substr(0, pos);
                std::string bodyStr = ruleStr.substr(pos + 2);
                
                // Trim
                auto trim = [](std::string& s) {
                    size_t start = s.find_first_not_of(" \t");
                    size_t end = s.find_last_not_of(" \t");
                    if (start == std::string::npos) {
                        s.clear();
                    } else {
                        s = s.substr(start, end - start + 1);
                    }
                };
                trim(headStr);
                trim(bodyStr);
                
                parser.setRuleMode(true);
                kbgdb::Fact head = parser.parse(headStr);
                
                // Parse body goals
                std::vector<kbgdb::Fact> body;
                std::string current;
                int parenDepth = 0;
                
                for (char c : bodyStr) {
                    if (c == '(') {
                        parenDepth++;
                        current += c;
                    } else if (c == ')') {
                        parenDepth--;
                        current += c;
                    } else if (c == ',' && parenDepth == 0) {
                        trim(current);
                        if (!current.empty()) {
                            body.push_back(parser.parse(current));
                        }
                        current.clear();
                    } else {
                        current += c;
                    }
                }
                trim(current);
                if (!current.empty()) {
                    body.push_back(parser.parse(current));
                }
                
                kbgdb::Rule rule(head, body);
                kb.addRule(rule);
                std::cout << "Added rule: " << rule.toString() << std::endl;
                
            } else if (line.substr(0, 3) == "?- ") {
                std::string queryStr = line.substr(3);
                
                auto results = kb.query(queryStr);
                
                if (results.empty()) {
                    std::cout << "false." << std::endl;
                } else {
                    for (const auto& binding : results) {
                        if (binding.bindings.empty()) {
                            std::cout << "true." << std::endl;
                        } else {
                            std::cout << binding.toString() << std::endl;
                        }
                    }
                }
            } else {
                std::cerr << "Unknown command. Type 'help' for available commands." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}
