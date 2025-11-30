// examples/simple_query.cpp
#include "common/fact.h"
#include "core/knowledge_base.h"
#include "query/query_engine.h"
#include "storage/rocksdb_provider.h"
#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <iostream>

DEFINE_string(rules_file, "rules.txt", "Path to rules file");
DEFINE_string(rocksdb_path, "", "Path to RocksDB database (optional)");
DEFINE_string(query, "", "Query to execute (e.g., 'parent(?X, mary')");

int main(int argc, char* argv[]) {
    // Initialize folly using RAII
    folly::Init init(&argc, &argv);
    
    try {
        // Validate required arguments
        if (FLAGS_query.empty()) {
            std::cerr << "Error: --query flag is required" << std::endl;
            std::cerr << "Example: ./simple_query_example --query=\"parent(?X, mary)\"" << std::endl;
            return 1;
        }
        
        // Create knowledge base
        std::cout << "Loading knowledge base from: " << FLAGS_rules_file << std::endl;
        auto kb = std::make_shared<kbgdb::KnowledgeBase>(FLAGS_rules_file);
        
        // Add RocksDB provider if path is specified
        if (!FLAGS_rocksdb_path.empty()) {
            std::cout << "Adding RocksDB provider: " << FLAGS_rocksdb_path << std::endl;
            kb->addExternalProvider(
                std::make_unique<kbgdb::RocksDBProvider>(FLAGS_rocksdb_path)
            );
        }
        
        // Create query engine
        kbgdb::QueryEngine engine(kb);
        
        // Execute query
        std::cout << "Executing query: " << FLAGS_query << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        auto future = engine.executeQuery(FLAGS_query);
        auto result = std::move(future).get();
        
        // Display results
        if (result.success) {
            std::cout << "Query successful!" << std::endl;
            std::cout << "Results:" << std::endl;
            
            if (result.bindings.empty()) {
                std::cout << "  No results found." << std::endl;
            } else {
                for (size_t i = 0; i < result.bindings.size(); ++i) {
                    std::cout << "  Result " << (i + 1) << ":" << std::endl;
                    for (const auto& [var, value] : result.bindings[i].bindings) {
                        std::cout << "    " << var << " = " << value << std::endl;
                    }
                }
            }
            
            std::cout << "\nJSON format:" << std::endl;
            std::cout << result.toJSON() << std::endl;
        } else {
            std::cout << "Query failed: " << result.error << std::endl;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
