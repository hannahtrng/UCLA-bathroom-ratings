#ifndef HANDLER_REGISTRY_H
#define HANDLER_REGISTRY_H

#include "handler_factory.h"
#include <string>
#include <map>
#include <memory>

class HandlerRegistry {
public:
    // Register a factory for a location path
    static void RegisterHandler(const std::string& path, std::shared_ptr<HandlerFactory> factory);
    
    // Get a factory instance for a location path
    // Returns nullptr if no factory is registered for the path
    static std::shared_ptr<HandlerFactory> GetFactory(const std::string& path);

private:
    // Map of location paths to created factories
    static std::map<std::string, std::shared_ptr<HandlerFactory>> _factories;
};

#endif // HANDLER_REGISTRY_H

