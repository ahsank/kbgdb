// src/http/server.h
#pragma once
#include "core/knowledge_base.h"
#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <memory>

namespace kbgdb {

class Server {
public:
    Server(uint16_t port, std::shared_ptr<KnowledgeBase> kb);
    
    void start();
    void stop();

private:
    uint16_t port_;
    std::shared_ptr<KnowledgeBase> kb_;
    std::unique_ptr<proxygen::HTTPServer> server_;
    folly::EventBase evb_;
};

} // namespace kbgdb
