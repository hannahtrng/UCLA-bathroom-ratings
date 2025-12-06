#ifndef HEALTH_REQUEST_HANDLER_H
#define HEALTH_REQUEST_HANDLER_H

#include <cstdlib>
#include <iostream>
#include <memory>

#include <string>
#include "request_handler.h"

class HealthRequestHandler : public RequestHandler {
  public:
    std::unique_ptr<response> handle_request(const request& request) override;
    std::string GetHandlerName() const override;
};

#endif // HEALTH_REQUEST_HANDLER_H

