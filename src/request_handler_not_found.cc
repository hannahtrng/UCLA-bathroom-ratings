#include "request_handler.h"
#include "response_builder.h"
#include "request_handler_not_found.h"

std::unique_ptr<response> NotFoundRequestHandler::handle_request(const request& request) {
  (void)request; 
  return ResponseBuilder::NotFound();
}

std::string NotFoundRequestHandler::GetHandlerName() const {
    return "NotFoundHandler";
}
