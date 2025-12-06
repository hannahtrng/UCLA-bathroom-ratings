#ifndef NOT_FOUND_REQUEST_HANDLER_H
#define NOT_FOUND_REQUEST_HANDLER_H

#include <cstdlib>
#include <iostream>
#include <memory>

#include <string>
#include "request_handler.h"

class NotFoundRequestHandler : public RequestHandler {
  public:
    std::unique_ptr<response> handle_request(const request& request) override;
    std::string GetHandlerName() const override;
};

#endif // NOT_FOUND_REQUEST_HANDLER_H
