#ifndef REQUEST_HANDLER_SLEEP_H
#define REQUEST_HANDLER_SLEEP_H

#include "config_parser.h"
#include "request_handler.h"

class SleepHandler : public RequestHandler {
public:
    virtual ~SleepHandler() = default;
    virtual std::unique_ptr<response> handle_request(const request &request);
    static std::unique_ptr<RequestHandler> create(const NginxConfig& config, const std::string& root_path);
    virtual std::string GetHandlerName() const { return "SleepHandler"; }
};

#endif // REQUEST_HANDLER_SLEEP_H
