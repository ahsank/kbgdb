// examples/simple_query.cpp
// A simple example demonstrating synchronous query execution

#include "core/knowledge_base.h"
#include "query/query_engine.h"
#include <iostream>
#include <string>

void printUsage(const char* program) {
    std::cerr << "Usage: " << program << " --rules_file <file> --query <query>" << std::endl;
    std::cerr << "Example: " << program << " --rules_file rules.txt --query \"grandparent(?X, ?Y)\"" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string rulesFile;
    std::string query;
    
    // Simple argument parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--rules_file" && i + 1 < argc) {
            rulesFile = argv[++i];
        } else if (arg == "--query" && i + 1 < argc) {
            query = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    if (rulesFile.empty() || query.empty()) {
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        std::cout << "Loading knowledge base from: " << rulesFile << std::endl;
        auto kb = std::make_shared<kbgdb::KnowledgeBase>(rulesFile);
        
        std::cout << "\nFacts and Rules loaded:" << std::endl;
        kb->printFacts();
        kb->printRules();
        
        std::cout << "\nExecuting query: " << query << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        kbgdb::QueryEngine engine(kb);
        auto result = engine.execute(query);
        
        if (result.success) {
            if (result.bindings.empty()) {
                std::cout << "No results found." << std::endl;
            } else {
                std::cout << "Found " << result.bindings.size() << " result(s):" << std::endl;
                for (size_t i = 0; i < result.bindings.size(); ++i) {
                    std::cout << "  Result " << (i + 1) << ": " 
                              << result.bindings[i].toString() << std::endl;
                }
            }
            
            std::cout << "\nJSON output:" << std::endl;
            std::cout << result.toJSON() << std::endl;
        } else {
            std::cerr << "Query failed: " << result.error << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
