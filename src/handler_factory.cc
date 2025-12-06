#include "handler_factory.h"
#include "request_handler_api.h"
#include "request_handler_echo.h"
#include "request_handler_file.h"
#include "request_handler_health.h"
#include "request_handler_not_found.h"
#include "request_handler_sleep.h"

std::unique_ptr<RequestHandler> EchoHandlerFactory::Create() {
    return std::make_unique<EchoRequestHandler>();
}

std::unique_ptr<RequestHandler> StaticHandlerFactory::Create() {
    // Get the root directory from args, default to current directory
    std::string root = ".";
    auto it = config_.args.find("root");
    if (it != config_.args.end()) {
        root = it->second;
    }
    return std::make_unique<FileRequestHandler>(root, config_.path);
}

std::unique_ptr<RequestHandler> NotFoundHandlerFactory::Create() {
    return std::make_unique<NotFoundRequestHandler>();
}

std::unique_ptr<RequestHandler> HealthHandlerFactory::Create() {
    return std::make_unique<HealthRequestHandler>();
}

ApiHandlerFactory::ApiHandlerFactory(const LocationConfig &config, std::shared_ptr<EntityStore> entity_store)
    : HandlerFactory(config), entity_store_(std::move(entity_store)) { }

std::unique_ptr<RequestHandler> ApiHandlerFactory::Create() {
    return std::make_unique<ApiRequestHandler>(entity_store_, config_.path);
}

std::unique_ptr<RequestHandler> SleepHandlerFactory::Create() {
    return std::make_unique<SleepHandler>();
}
