#include "query/query_engine.h"
#include <sstream>

namespace kbgdb {

std::string QueryResult::toJSON() const {
    std::ostringstream oss;
    
    if (!success) {
        oss << R"({"success": false, "error": ")" << error << R"("})";
        return oss.str();
    }
    
    oss << R"({"success": true, "bindings": [)";
    
    for (size_t i = 0; i < bindings.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "{";
        
        bool first = true;
        for (const auto& [var, value] : bindings[i].bindings) {
            if (!first) oss << ", ";
            oss << R"(")" << var << R"(": ")" << value.toString() << R"(")";
            first = false;
        }
        
        oss << "}";
    }
    
    oss << "]}";
    return oss.str();
}

QueryEngine::QueryEngine(std::shared_ptr<KnowledgeBase> kb)
    : kb_(std::move(kb)) {
}

QueryResult QueryEngine::execute(const std::string& queryStr) {
    QueryResult result;
    
    try {
        result.bindings = kb_->query(queryStr);
        result.success = true;
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }
    
    return result;
}

} // namespace kbgdb
