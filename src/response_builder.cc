#include "response_builder.h"
#include <boost/log/trivial.hpp>

std::unique_ptr<response> ResponseBuilder::FormatResponse(const std::string& status_code,
                                                          const std::string& content_type,
                                                          const std::string& body) {
    BOOST_LOG_TRIVIAL(info) << "Building response with status: " << status_code 
                            << ", Content-Type: " << content_type;
    auto resp = std::make_unique<response>();
    resp->http_version = "1.1";
    resp->status_code = status_code;
    resp->content_type = content_type;
    resp->body = body;
    resp->content_length = std::to_string(body.size());
    resp->headers.push_back("Content-Type: " + content_type);
    resp->headers.push_back("Content-Length: " + resp->content_length);
    resp->headers.push_back("Connection: close");
    return resp;
}

std::unique_ptr<response> ResponseBuilder::BuildResponse(const std::string& status_code,
                                                         const std::string& content_type,
                                                         const std::string& body) {
    return FormatResponse(status_code, content_type, body);
}

std::unique_ptr<response> ResponseBuilder::Ok(const std::string& body, const std::string& content_type) {
    return FormatResponse("200 OK", content_type, body);
}

std::unique_ptr<response> ResponseBuilder::NotFound(const std::string& message) {
    return FormatResponse("404 Not Found", "text/plain", message);
}

std::unique_ptr<response> ResponseBuilder::Forbidden(const std::string& message) {
    return FormatResponse("403 Forbidden", "text/plain", message);
}

std::unique_ptr<response> ResponseBuilder::BadRequest(const std::string& message) {
    return FormatResponse("400 Bad Request", "text/plain", message);
}

std::unique_ptr<response> ResponseBuilder::InternalServerError(const std::string& message) {
    return FormatResponse("500 Internal Server Error", "text/plain", message);
}

std::unique_ptr<response> ResponseBuilder::MethodNotAllowed(const std::string& message) {
    return FormatResponse("405 Method Not Allowed", "text/plain", message);
}
