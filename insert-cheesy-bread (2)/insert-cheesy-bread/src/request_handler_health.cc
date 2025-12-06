#include "request_handler.h"
#include "response_builder.h"
#include "request_handler_health.h"

std::unique_ptr<response> HealthRequestHandler::handle_request(const request& request) {
  (void)request; 
  return ResponseBuilder::Ok("OK", "text/plain");
}

std::string HealthRequestHandler::GetHandlerName() const {
    return "HealthHandler";
}

