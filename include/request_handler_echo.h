#ifndef ECHO_REQUEST_HANDLER_H
#define ECHO_REQUEST_HANDLER_H

#include <cstdlib>
#include <iostream>
#include <memory>

#include <string>
#include "request_handler.h"

class EchoRequestHandler : public RequestHandler {
  public:
    std::unique_ptr<response> handle_request(const request& request) override;
    std::string GetHandlerName() const override;
};

#endif // ECHO_REQUEST_HANDLER_H