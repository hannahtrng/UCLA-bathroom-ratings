#ifndef RESPONSE_BUILDER_H
#define RESPONSE_BUILDER_H

#include <string>
#include <memory>
#include "request_handler.h"

class ResponseBuilder {
public:
    static std::unique_ptr<response> BuildResponse(const std::string& status_code,
                                                   const std::string& content_type,
                                                   const std::string& body);
    
    static std::unique_ptr<response> Ok(const std::string& body, 
                                        const std::string& content_type = "text/plain");
    
    static std::unique_ptr<response> NotFound(const std::string& message = "404 Not Found");
    
    static std::unique_ptr<response> Forbidden(const std::string& message = "403 Forbidden");
    
    static std::unique_ptr<response> BadRequest(const std::string& message = "400 Bad Request");
    
    static std::unique_ptr<response> InternalServerError(const std::string& message = "500 Internal Server Error");
    
    static std::unique_ptr<response> MethodNotAllowed(const std::string& message = "405 Method Not Allowed");

private:
    static std::unique_ptr<response> FormatResponse(const std::string& status_code,
                                                    const std::string& content_type,
                                                    const std::string& body);
};

#endif // RESPONSE_BUILDER_H

