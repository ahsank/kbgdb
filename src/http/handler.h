// src/http/handler.h
#pragma once
#include "core/knowledge_base.h"
#include <proxygen/httpserver/RequestHandler.h>
#include <folly/io/IOBuf.h>
#include <memory>

namespace kbgdb {

class QueryHandler : public proxygen::RequestHandler {
public:
    explicit QueryHandler(std::shared_ptr<KnowledgeBase> kb);

    void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
    void onEOM() noexcept override;
    void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override;
    void requestComplete() noexcept override;
    void onError(proxygen::ProxygenError err) noexcept override;

private:
    std::shared_ptr<KnowledgeBase> kb_;
    std::unique_ptr<folly::IOBuf> body_;
};

} // namespace kbgdb
