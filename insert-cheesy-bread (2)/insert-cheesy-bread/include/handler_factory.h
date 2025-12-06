#ifndef HANDLER_FACTORY_H
#define HANDLER_FACTORY_H

#include "entity_store.h"
#include "request_handler.h"
#include "server_config.h"
#include <memory>

// Base factory class - stores config and creates handlers
class HandlerFactory {
protected:
    LocationConfig config_;
    
public:
    HandlerFactory(const LocationConfig& config) : config_(config) {}
    virtual ~HandlerFactory() = default;
    
    // Create a handler instance using the stored config
    virtual std::unique_ptr<RequestHandler> Create() = 0;
};

// Factory for echo handlers
class EchoHandlerFactory : public HandlerFactory {
public:
    EchoHandlerFactory(const LocationConfig& config) : HandlerFactory(config) {}
    
    std::unique_ptr<RequestHandler> Create() override;
};

// Factory for static/file handlers
class StaticHandlerFactory : public HandlerFactory {
public:
    StaticHandlerFactory(const LocationConfig& config) : HandlerFactory(config) {}
    
    std::unique_ptr<RequestHandler> Create() override;
};

// Factory for not found handlers
class NotFoundHandlerFactory : public HandlerFactory {
public:
    NotFoundHandlerFactory(const LocationConfig& config) : HandlerFactory(config) {}
    
    std::unique_ptr<RequestHandler> Create() override;
};

// Factory for health handlers
class HealthHandlerFactory : public HandlerFactory {
public:
    HealthHandlerFactory(const LocationConfig& config) : HandlerFactory(config) {}
    
    std::unique_ptr<RequestHandler> Create() override;
};

// Factory for sleep handlers
class SleepHandlerFactory : public HandlerFactory {
public:
    SleepHandlerFactory(const LocationConfig& config) : HandlerFactory(config) {}
    
    std::unique_ptr<RequestHandler> Create() override;
};

// Factory for api handler
class ApiHandlerFactory : public HandlerFactory {
public:
    ApiHandlerFactory(const LocationConfig& config, std::shared_ptr<EntityStore> entity_store);
    
    std::unique_ptr<RequestHandler> Create() override;

private:
    std::shared_ptr<EntityStore> entity_store_;
};


#endif // HANDLER_FACTORY_H
