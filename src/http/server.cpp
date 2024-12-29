#include "http/server.h"
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/json.h>
#include <folly/executors/GlobalExecutor.h>
#include <fmt/format.h>
#include <iostream>

namespace kbgdb {

class Server::ConnectionHandler : public folly::AsyncTransportWrapper::ReadCallback,
                                public folly::AsyncTransportWrapper::WriteCallback {
public:
    ConnectionHandler(folly::AsyncTransportWrapper::UniquePtr sock,
                      std::shared_ptr<KnowledgeBase> kb)
        : socket_(dynamic_cast<folly::AsyncSocket*>(sock.release())), kb_(kb) {
        if (!socket_) {
            throw std::invalid_argument("Socket is not of type AsyncSocket");
        }
        socket_->setReadCB(this);
    }

    void getReadBuffer(void** bufReturn, size_t* lenReturn) override {
        auto res = readBuffer_.preallocate(4096, 4096);
        *bufReturn = res.first;
        *lenReturn = res.second;
    }

    void readDataAvailable(size_t len) noexcept override {
        readBuffer_.postallocate(len);
        
        // Convert IOBufQueue to string
        std::string data;
        auto buf = readBuffer_.move();
        if (buf) {
            data.reserve(buf->computeChainDataLength());
            for (auto& range : *buf) {
                data.append(reinterpret_cast<const char*>(range.data()), range.size());
            }
        }
        
        // Simple HTTP parser
        if (data.find("POST /api/query") != std::string::npos) {
            size_t contentPos = data.find("\r\n\r\n");
            if (contentPos != std::string::npos) {
                std::string body = data.substr(contentPos + 4);
                handleQuery(body);
            }
        } else {
            // Send 404 for non-query endpoints
            std::string response = "HTTP/1.1 404 Not Found\r\n"
                                 "Content-Length: 0\r\n"
                                 "\r\n";
            auto buf = folly::IOBuf::copyBuffer(response);
            socket_->writeChain(this, std::move(buf));
        }
    }

    void readEOF() noexcept override {
        delete this;
    }

    void readErr(const folly::AsyncSocketException& ex) noexcept override {
        delete this;
    }

    void writeSuccess() noexcept override {}
    void writeErr(size_t, const folly::AsyncSocketException&) noexcept override {
        delete this;
    }

private:
    folly::AsyncSocket* socket_; 
    std::shared_ptr<KnowledgeBase> kb_;
    folly::IOBufQueue readBuffer_{folly::IOBufQueue::cacheChainLength()};

    void handleQuery(const std::string& body) {
        try {
            auto json = folly::parseJson(body);
            if (!json.count("query")) {
                sendErrorResponse(400, "Missing 'query' field");
                return;
            }

            std::string queryStr = json["query"].getString();
            kb_->query(queryStr)
                .via(folly::getGlobalCPUExecutor())
                .thenValue([this](std::vector<BindingSet> results) {
                    // Convert results to JSON
                    folly::dynamic resultJson = folly::dynamic::array;
                    for (const auto& binding : results) {
                        folly::dynamic bindingJson = folly::dynamic::object;
                        for (const auto& [var, value] : binding.bindings) {
                            bindingJson[var] = value;
                        }
                        resultJson.push_back(std::move(bindingJson));
                    }

                    sendJsonResponse(200, folly::toJson(resultJson));
                })
                .thenError<std::exception>([this](const std::exception& e) {
                    sendErrorResponse(500, e.what());
                });

        } catch (const std::exception& e) {
            sendErrorResponse(400, e.what());
        }
    }

    void sendJsonResponse(int status, const std::string& json) {
        std::string response = fmt::format(
            "HTTP/1.1 {} OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: {}\r\n"
            "\r\n"
            "{}", status, json.length(), json);

        auto buf = folly::IOBuf::copyBuffer(response);
        socket_->writeChain(this, std::move(buf));
    }

    void sendErrorResponse(int status, const std::string& error) {
        folly::dynamic errorJson = folly::dynamic::object;
        errorJson["error"] = error;
        sendJsonResponse(status, folly::toJson(errorJson));
    }
};

Server::Server(uint16_t port, std::shared_ptr<KnowledgeBase> kb)
    : port_(port)
    , kb_(std::move(kb)) {
}

void Server::start() {
    serverSocket_ = folly::AsyncServerSocket::newSocket(&evb_);
    serverSocket_->bind(port_);
    serverSocket_->listen(128);
    serverSocket_->addAcceptCallback(
        this,
        &evb_);
    serverSocket_->startAccepting();
    
    std::cout << "Server started on port " << port_ << std::endl;
    evb_.loopForever();
}

void Server::stop() {
    if (serverSocket_) {
        serverSocket_->stopAccepting();
    }
    evb_.terminateLoopSoon();
}

void Server::connectionAccepted(folly::NetworkSocket sock,
                               const folly::SocketAddress& clientAddr,
                               AcceptInfo info) noexcept {
    auto socket = folly::AsyncSocket::UniquePtr(
        new folly::AsyncSocket(&evb_, sock));
        
    auto handler = new ConnectionHandler(std::move(socket), kb_);
    (void)handler; // Suppress unused variable warning
}

} // namespace kbgdb
