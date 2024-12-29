#pragma once
#include "core/knowledge_base.h"
#include <folly/io/async/EventBase.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/AsyncSocket.h>
#include <memory>
#include <iostream>

namespace kbgdb {

class Server : public folly::AsyncServerSocket::AcceptCallback {
public:
    Server(uint16_t port, std::shared_ptr<KnowledgeBase> kb);
    
    void start();
    void stop();

private:
    uint16_t port_;
    std::shared_ptr<KnowledgeBase> kb_;
    folly::EventBase evb_;
    std::shared_ptr<folly::AsyncServerSocket> serverSocket_;
    
    class ConnectionHandler;

    // AcceptCallback interface implementation
    void connectionAccepted(folly::NetworkSocket sock,
                          const folly::SocketAddress& clientAddr,
                          AcceptInfo info) noexcept override;
    
    void acceptError(const std::exception& ex) noexcept override { 
        std::cerr << "Accept error: " << ex.what() << std::endl;
    }
};

} // namespace kbgdb