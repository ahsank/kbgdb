// src/http/handler.cpp
#include "http/handler.h"
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/executors/IOExecutor.h>
#include <proxygen/httpserver/ResponseBuilder.h>

namespace kbgdb {

QueryHandler::QueryHandler(std::shared_ptr<KnowledgeBase> kb)
    : kb_(std::move(kb)) {
}

void QueryHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {
    if (headers->getMethod() != proxygen::HTTPMethod::POST) {
        proxygen::ResponseBuilder(downstream_)
            .status(400, "Bad Request")
            .body("Only POST method is supported")
            .sendWithEOM();
        return;
    }
}

void QueryHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {
    if (body_) {
        body_->prependChain(std::move(body));
    } else {
        body_ = std::move(body);
    }
}

void QueryHandler::onEOM() noexcept {
    try {
        std::string bodyStr = body_ ? body_->moveToFbString().toStdString() : "";
        auto jsonBody = folly::parseJson(bodyStr);
        
        if (!jsonBody.count("query")) {
            proxygen::ResponseBuilder(downstream_)
                .status(400, "Bad Request")
                .body("Missing 'query' field")
                .sendWithEOM();
            return;
        }

        std::string queryStr = jsonBody["query"].getString();
        
        // Execute query asynchronously
        kb_->query(queryStr)
            .via(folly::getGlobalIOExecutor()->getEventBase())
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

                proxygen::ResponseBuilder(downstream_)
                    .status(200, "OK")
                    .header("Content-Type", "application/json")
                    .body(folly::toJson(resultJson))
                    .sendWithEOM();
            })
            .thenError<std::exception>([this](const std::exception& e) {
                proxygen::ResponseBuilder(downstream_)
                    .status(500, "Internal Server Error")
                    .body(e.what())
                    .sendWithEOM();
            });

    } catch (const std::exception& e) {
        proxygen::ResponseBuilder(downstream_)
            .status(400, "Bad Request")
            .body(e.what())
            .sendWithEOM();
    }
}

void QueryHandler::onUpgrade(proxygen::UpgradeProtocol) noexcept {
    // We don't support upgrades
}

void QueryHandler::requestComplete() noexcept {
    delete this;
}

void QueryHandler::onError(proxygen::ProxygenError err) noexcept {
    delete this;
}

} // namespace kbgdb
