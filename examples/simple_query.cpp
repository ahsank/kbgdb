// examples/service.cpp
#include "common/fact.h"
#include "core/knowledge_base.h"
#include "http/server.h"
#include "storage/rocksdb_provider.h"
#include <folly/init/Init.h>
#include <gflags/gflags.h>
#include <iostream>

DEFINE_int32(port, 8080, "Server port");
DEFINE_string(rules_file, "rules.txt", "Path to rules file");
DEFINE_string(rocksdb_path, "", "Path to RocksDB database (optional)");

int main(int argc, char* argv[]) {
    // Initialize folly using RAII
    folly::Init init(&argc, &argv);
    
    try {
        // Create knowledge base
        auto kb = std::make_shared<kbgdb::KnowledgeBase>(FLAGS_rules_file);
        
        // Add RocksDB provider if path is specified
        if (!FLAGS_rocksdb_path.empty()) {
            kb->addExternalProvider(
                std::make_unique<kbgdb::RocksDBProvider>(FLAGS_rocksdb_path)
            );
        }
        
        // Create and start server
        kbgdb::Server server(FLAGS_port, kb);
        
        std::cout << "Starting KBGDB server on port " << FLAGS_port << std::endl;
        server.start();
        
        // Wait for signal to shutdown
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        server.stop();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
