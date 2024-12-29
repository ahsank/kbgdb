// src/http/server.cpp
#include "http/server.h"
#include "http/handler.h"
#include <folly/io/async/EventBaseManager.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

namespace kbgdb {

class HandlerFactory : public proxygen::RequestHandlerFactory {
public:
    explicit HandlerFactory(std::shared_ptr<KnowledgeBase> kb)
        : kb_(std::move(kb)) {}

    void onServerStart(folly::EventBase*) noexcept override {}
    void onServerStop() noexcept override {}

    proxygen::RequestHandler* onRequest(proxygen::RequestHandler*, 
                                      proxygen::HTTPMessage*) noexcept override {
        return new QueryHandler(kb_);
    }

private:
    std::shared_ptr<KnowledgeBase> kb_;
};

Server::Server(uint16_t port, std::shared_ptr<KnowledgeBase> kb)
    : port_(port)
    , kb_(std::move(kb)) {
}

void Server::start() {
    std::vector<proxygen::HTTPServer::IPConfig> IPs = {
        {folly::SocketAddress("0.0.0.0", port_), proxygen::HTTPServer::Protocol::HTTP}
    };

    proxygen::HTTPServerOptions options;
    options.threads = std::thread::hardware_concurrency();
    options.idleTimeout = std::chrono::milliseconds(60000);
    options.handlerFactories = proxygen::RequestHandlerChain()
        .addThen<HandlerFactory>(kb_)
        .build();

    server_ = std::make_unique<proxygen::HTTPServer>(std::move(options));
    server_->bind(IPs);

    // Start HTTPServer mainloop in a separate thread
    server_->start();
}

void Server::stop() {
    server_->stop();
}

} // namespace kbgdb
