#include "request_handler_api.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "entity_store_validation.h"
#include "response_builder.h"

namespace {
    const char *kJsonContentType = "application/json";
}

// This code was partially created with the help of GPT-5.1-CODEX

ApiRequestHandler::ApiRequestHandler(
    std::shared_ptr<EntityStore> entity_store, const std::string &url_prefix)
    : entity_store_(std::move(entity_store)), url_prefix_(url_prefix) {
    if (!entity_store_) {
        throw std::invalid_argument("entity_store must not be null");
    }
}

std::unique_ptr<response> ApiRequestHandler::handle_request(const request &request) {
    if (!entity_store_) {
        return ResponseBuilder::InternalServerError(
            "Entity store is unavailable");
    }

    std::vector<std::string> segments = ExtractSegments(request.url);
    if (segments.empty()) {
        return ResponseBuilder::BadRequest("Entity type required");
    }

    const std::string &entity_name = segments.front();
    if (!EntityStoreValidation::IsValidEntityName(entity_name)) {
        return ResponseBuilder::BadRequest("Invalid entity name");
    }

    if (request.method == "POST") {
        if (segments.size() != 1) {
            return ResponseBuilder::BadRequest("POST target must be /Entity");
        }
        return HandlePost(entity_name, request.body);
    }

    if (request.method == "GET") {
        return HandleGet(entity_name, segments);
    }

    if (request.method == "PUT") {
        if (segments.size() != 2) {
            return ResponseBuilder::BadRequest("PUT target must be /Entity/{id}");
        }
        int id;
        if (!EntityStoreValidation::ParseId(segments[1], id)) {
            return ResponseBuilder::BadRequest("Invalid ID");
        }
        return HandlePut(entity_name, id, request.body);
    }

    if (request.method == "DELETE") {
        if (segments.size() != 2) {
            return ResponseBuilder::BadRequest("DELETE target must be /Entity/{id}");
        }
        int id;
        if (!EntityStoreValidation::ParseId(segments[1], id)) {
            return ResponseBuilder::BadRequest("Invalid ID");
        }
        return HandleDelete(entity_name, id);
    }

    return ResponseBuilder::MethodNotAllowed("Unsupported method");
}

std::unique_ptr<response> ApiRequestHandler::HandlePost(const std::string &entity_name,
                                                        const std::string &body) {
    if (!EntityStoreValidation::IsValidJson(body)) {
        return ResponseBuilder::BadRequest("Body is not valid JSON");
    }
    int id = entity_store_->Create(entity_name, body);
    if (id < 0) {
        return ResponseBuilder::InternalServerError("Failed to create entity");
    }
    std::ostringstream json;
    json << "{\"id\":" << id << "}";
    return ResponseBuilder::Ok(json.str(), kJsonContentType);
}

std::unique_ptr<response>
ApiRequestHandler::HandleGet(const std::string &entity_name,
                             const std::vector<std::string> &segments) {

    // GET /Entity - listing all IDs for the entity
    if (segments.size() == 1) {
        std::vector<int> ids = entity_store_->List(entity_name);
        std::sort(ids.begin(), ids.end());
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < ids.size(); ++i) {
            if (i != 0) {
                json << ",";
            }
            json << ids[i];
        }
        json << "]";
        return ResponseBuilder::Ok(json.str(), kJsonContentType);
    }

    // GET /Entity/{id} - retrieving a specific entity by ID
    int id;
    if (!EntityStoreValidation::ParseId(segments[1], id)) {
        return ResponseBuilder::BadRequest("Invalid ID");
    }

    std::string data;
    if (!entity_store_->Read(entity_name, id, data)) {
        return ResponseBuilder::NotFound("Entity not found");
    }
    return ResponseBuilder::Ok(data, kJsonContentType);
}

std::unique_ptr<response> ApiRequestHandler::HandlePut(const std::string &entity_name, int id,
                                                       const std::string &body) {
    if (!EntityStoreValidation::IsValidJson(body)) {
        return ResponseBuilder::BadRequest("Body is not valid JSON");
    }
    if (!entity_store_->Update(entity_name, id, body)) {
        return ResponseBuilder::InternalServerError("Failed to update entity");
    }
    std::ostringstream json;
    json << "{\"id\":" << id << "}";
    return ResponseBuilder::Ok(json.str(), kJsonContentType);
}

std::unique_ptr<response> ApiRequestHandler::HandleDelete(const std::string &entity_name, int id) {
    if (!entity_store_->Delete(entity_name, id)) {
        return ResponseBuilder::NotFound("Entity not found");
    }
    return ResponseBuilder::Ok("{\"deleted\":true}", kJsonContentType);
}

std::vector<std::string> ApiRequestHandler::ExtractSegments(const std::string &raw_url) const {
    std::string path = raw_url;
    auto query_pos = path.find('?');
    if (query_pos != std::string::npos) {
        path = path.substr(0, query_pos);
    }
    if (!url_prefix_.empty() && path.rfind(url_prefix_, 0) == 0) {
        size_t prefix_len = url_prefix_.size();
        bool exact_match = path.size() == prefix_len;
        bool boundary = !exact_match && path[prefix_len] == '/';
        if (exact_match || boundary) {
            path = path.substr(prefix_len);
        }
    }
    while (!path.empty() && path.front() == '/') {
        path.erase(path.begin());
    }
    while (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    std::vector<std::string> segments;
    if (path.empty()) {
        return segments;
    }
    std::stringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, '/')) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    return segments;
}

std::string ApiRequestHandler::GetHandlerName() const {
    return "ApiHandler";
}
