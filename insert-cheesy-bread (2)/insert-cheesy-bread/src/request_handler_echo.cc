#include <cstdlib>
#include <iostream>

#include "request_handler.h"
#include "request_handler_echo.h"
#include "response_builder.h"

std::unique_ptr<response> EchoRequestHandler::handle_request(const request& request) {
    return ResponseBuilder::Ok(request.raw_request, "text/plain");
}

std::string EchoRequestHandler::GetHandlerName() const {
    return "EchoHandler";
}
